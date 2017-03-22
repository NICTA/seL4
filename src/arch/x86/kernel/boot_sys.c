/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <config.h>
#include <util.h>
#include <machine/io.h>
#include <arch/machine.h>
#include <arch/kernel/apic.h>
#include <arch/kernel/cmdline.h>
#include <arch/kernel/boot.h>
#include <arch/kernel/boot_sys.h>
#include <arch/kernel/smp_sys.h>
#include <arch/kernel/vspace.h>
#include <arch/kernel/elf.h>
#include <smp/lock.h>
#include <arch/linker.h>
#include <plat/machine/acpi.h>
#include <plat/machine/devices.h>
#include <plat/machine/pic.h>
#include <plat/machine/ioapic.h>
#include <arch/api/bootinfo_types.h>

/* addresses defined in linker script */
/* need a fake array to get the pointer from the linker script */

/* start/end of CPU boot code */
extern char boot_cpu_start[1];
extern char boot_cpu_end[1];

/* start/end of boot stack */
extern char boot_stack_bottom[1];
extern char boot_stack_top[1];

/* locations in kernel image */
extern char ki_boot_end[1];
extern char ki_end[1];

#ifdef CONFIG_PRINTING
/* kernel entry point */
extern char _start[1];
#endif

/* constants */

#define HIGHMEM_PADDR 0x100000

/* type definitions (directly corresponding to abstract specification) */

typedef struct boot_state {
    p_region_t   avail_p_reg; /* region of available physical memory on platform */
    p_region_t   ki_p_reg;    /* region where the kernel image is in */
    ui_info_t    ui_info;     /* info about userland images */
    uint32_t     num_ioapic;  /* number of IOAPICs detected */
    paddr_t      ioapic_paddr[CONFIG_MAX_NUM_IOAPIC];
    uint32_t     num_drhu; /* number of IOMMUs */
    paddr_t      drhu_list[MAX_NUM_DRHU]; /* list of physical addresses of the IOMMUs */
    acpi_rmrr_list_t rmrr_list;
    uint32_t     num_cpus;    /* number of detected cpus */
    cpu_id_t     cpus[CONFIG_MAX_NUM_NODES];
    mem_p_regs_t mem_p_regs;  /* physical memory regions */
    seL4_X86_BootInfo_VBE vbe_info; /* Potential VBE information from multiboot */
} boot_state_t;

BOOT_BSS
boot_state_t boot_state;

/* global variables (not covered by abstract specification) */

BOOT_BSS
cmdline_opt_t cmdline_opt;

/* check the module occupies in a contiguous physical memory region */
BOOT_CODE static bool_t
module_paddr_region_valid(paddr_t pa_start, paddr_t pa_end)
{
    int i = 0;
    for (i = 0; i < boot_state.mem_p_regs.count; i++) {
        paddr_t start = boot_state.mem_p_regs.list[i].start;
        paddr_t end = boot_state.mem_p_regs.list[i].end;
        if (pa_start >= start && pa_end < end) {
            return true;
        }
    }
    return false;
}

/* functions not modeled in abstract specification */

BOOT_CODE static paddr_t
find_load_paddr(paddr_t min_paddr, word_t image_size)
{
    int i;

    for (i = 0; i < boot_state.mem_p_regs.count; i++) {
        paddr_t start = MAX(min_paddr, boot_state.mem_p_regs.list[i].start);
        paddr_t end = boot_state.mem_p_regs.list[i].end;
        word_t region_size = end - start;

        if (region_size >= image_size) {
            return start;
        }
    }

    return 0;
}

BOOT_CODE static paddr_t
load_boot_module(multiboot_module_t* boot_module, paddr_t load_paddr)
{
    v_region_t v_reg;
    word_t entry;
    Elf_Header_t* elf_file = (Elf_Header_t*)(word_t)boot_module->start;

    if (!elf_checkFile(elf_file)) {
        printf("Boot module does not contain a valid ELF image\n");
        return 0;
    }

    v_reg = elf_getMemoryBounds(elf_file);
    entry = elf_file->e_entry;

    if (v_reg.end == 0) {
        printf("ELF image in boot module does not contain any segments\n");
        return 0;
    }
    v_reg.end = ROUND_UP(v_reg.end, PAGE_BITS);

    printf("size=0x%lx v_entry=%p v_start=%p v_end=%p ",
           v_reg.end - v_reg.start,
           (void*)entry,
           (void*)v_reg.start,
           (void*)v_reg.end
          );

    if (!IS_ALIGNED(v_reg.start, PAGE_BITS)) {
        printf("Userland image virtual start address must be 4KB-aligned\n");
        return 0;
    }
    if (v_reg.end + 2 * BIT(PAGE_BITS) > PPTR_USER_TOP) {
        /* for IPC buffer frame and bootinfo frame, need 2*4K of additional userland virtual memory */
        printf("Userland image virtual end address too high\n");
        return 0;
    }
    if ((entry < v_reg.start) || (entry >= v_reg.end)) {
        printf("Userland image entry point does not lie within userland image\n");
        return 0;
    }

    load_paddr = find_load_paddr(load_paddr, v_reg.end - v_reg.start);
    assert(load_paddr);

    /* fill ui_info struct */
    boot_state.ui_info.pv_offset = load_paddr - v_reg.start;
    boot_state.ui_info.p_reg.start = load_paddr;
    load_paddr += v_reg.end - v_reg.start;
    boot_state.ui_info.p_reg.end = load_paddr;
    boot_state.ui_info.v_entry = entry;

    printf("p_start=0x%lx p_end=0x%lx\n",
           boot_state.ui_info.p_reg.start,
           boot_state.ui_info.p_reg.end
          );

    if (!module_paddr_region_valid(
                boot_state.ui_info.p_reg.start,
                boot_state.ui_info.p_reg.end)) {
        printf("End of loaded userland image lies outside of usable physical memory\n");
        return 0;
    }

    /* initialise all initial userland memory and load potentially sparse ELF image */
    memzero(
        (void*)boot_state.ui_info.p_reg.start,
        boot_state.ui_info.p_reg.end - boot_state.ui_info.p_reg.start
    );
    elf_load(elf_file, boot_state.ui_info.pv_offset);

    return load_paddr;
}

static BOOT_CODE bool_t
try_boot_sys_node(cpu_id_t cpu_id)
{
    p_region_t boot_mem_reuse_p_reg;

    if (!map_kernel_window(
                boot_state.num_ioapic,
                boot_state.ioapic_paddr,
                boot_state.num_drhu,
                boot_state.drhu_list
            )) {
        return false;
    }
    setCurrentVSpaceRoot(kpptr_to_paddr(X86_GLOBAL_VSPACE_ROOT), 0);
    /* Sync up the compilers view of the world here to force the PD to actually
     * be set *right now* instead of delayed */
    asm volatile("" ::: "memory");

    /* reuse boot code/data memory */
    boot_mem_reuse_p_reg.start = PADDR_LOAD;
    boot_mem_reuse_p_reg.end = (paddr_t)ki_boot_end - KERNEL_BASE_OFFSET;

    /* initialise the CPU */
    if (!init_cpu(config_set(CONFIG_IRQ_IOAPIC) ? 1 : 0)) {
        return false;
    }

    /* initialise NDKS and kernel heap */
    if (!init_sys_state(
                cpu_id,
                boot_state.mem_p_regs,
                boot_state.ui_info,
                boot_mem_reuse_p_reg,
                /* parameters below not modeled in abstract specification */
                boot_state.num_drhu,
                boot_state.drhu_list,
                &boot_state.rmrr_list,
                &boot_state.vbe_info
            )) {
        return false;
    }

    return true;
}

static BOOT_CODE bool_t
add_mem_p_regs(p_region_t reg)
{
    if (reg.end > PADDR_TOP) {
        reg.end = PADDR_TOP;
    }
    if (reg.start > PADDR_TOP) {
        reg.start = PADDR_TOP;
    }
    if (reg.start == reg.end) {
        /* Return true here as it's not an error for there to exist memory outside the kernel window,
         * we're just going to ignore it and leave it to be given out as device memory */
        return true;
    }
    if (boot_state.mem_p_regs.count == MAX_NUM_FREEMEM_REG) {
        printf("Dropping memory region 0x%lx-0x%lx, try increasing MAX_NUM_FREEMEM_REG\n", reg.start, reg.end);
        return false;
    }
    printf("Adding physical memory region 0x%lx-0x%lx\n", reg.start, reg.end);
    boot_state.mem_p_regs.list[boot_state.mem_p_regs.count] = reg;
    boot_state.mem_p_regs.count++;
    return add_allocated_p_region(reg);
}

/*
 * the code relies that the GRUB provides correct information
 * about the actual physical memory regions.
 */
static BOOT_CODE bool_t
parse_mem_map(uint32_t mmap_length, uint32_t mmap_addr)
{
    multiboot_mmap_t *mmap = (multiboot_mmap_t *)((word_t)mmap_addr);
    printf("Parsing GRUB physical memory map\n");

    while ((word_t)mmap < (word_t)(mmap_addr + mmap_length)) {
        uint64_t mem_start = mmap->base_addr;
        uint64_t mem_length = mmap->length;
        uint32_t type = mmap->type;
        if (mem_start != (uint64_t)(word_t)mem_start) {
            printf("\tPhysical memory region not addressable\n");
        } else {
            printf("\tPhysical Memory Region from %lx size %lx type %d\n", (long)mem_start, (long)mem_length, type);
            if (type == MULTIBOOT_MMAP_USEABLE_TYPE && mem_start >= HIGHMEM_PADDR) {
                if (!add_mem_p_regs((p_region_t) {
                mem_start, mem_start + mem_length
            })) {
                    return false;
                }
            }
        }
        mmap++;
    }
    return true;
}

static BOOT_CODE bool_t
is_compiled_for_microarchitecture(void)
{
    word_t microarch_generation = 0;
    x86_cpu_identity_t *model_info = x86_cpuid_get_model_info();

    if (config_set(CONFIG_ARCH_X86_SKYLAKE) ) {
        microarch_generation = 7;
    } else if (config_set(CONFIG_ARCH_X86_BROADWELL) ) {
        microarch_generation = 6;
    } else if (config_set(CONFIG_ARCH_X86_HASWELL) ) {
        microarch_generation = 5;
    } else if (config_set(CONFIG_ARCH_X86_IVY) ) {
        microarch_generation = 4;
    } else if (config_set(CONFIG_ARCH_X86_SANDY) ) {
        microarch_generation = 3;
    } else if (config_set(CONFIG_ARCH_X86_WESTMERE) ) {
        microarch_generation = 2;
    } else if (config_set(CONFIG_ARCH_X86_NEHALEM) ) {
        microarch_generation = 1;
    }

    switch (model_info->model) {
    case SKYLAKE_1_MODEL_ID:
    case SKYLAKE_2_MODEL_ID:
        if (microarch_generation > 7) {
            return false;
        }
        break;

    case BROADWELL_1_MODEL_ID:
    case BROADWELL_2_MODEL_ID:
    case BROADWELL_3_MODEL_ID:
    case BROADWELL_4_MODEL_ID:
    case BROADWELL_5_MODEL_ID:
        if (microarch_generation > 6) {
            return false;
        }
        break;

    case HASWELL_1_MODEL_ID:
    case HASWELL_2_MODEL_ID:
    case HASWELL_3_MODEL_ID:
    case HASWELL_4_MODEL_ID:
        if (microarch_generation > 5) {
            return false;
        }
        break;

    case IVY_BRIDGE_1_MODEL_ID:
    case IVY_BRIDGE_2_MODEL_ID:
    case IVY_BRIDGE_3_MODEL_ID:
        if (microarch_generation > 4) {
            return false;
        }
        break;

    case SANDY_BRIDGE_1_MODEL_ID:
    case SANDY_BRIDGE_2_MODEL_ID:
        if (microarch_generation > 3) {
            return false;
        }
        break;

    case WESTMERE_1_MODEL_ID:
    case WESTMERE_2_MODEL_ID:
    case WESTMERE_3_MODEL_ID:
        if (microarch_generation > 2) {
            return false;
        }
        break;

    case NEHALEM_1_MODEL_ID:
    case NEHALEM_2_MODEL_ID:
    case NEHALEM_3_MODEL_ID:
        if (microarch_generation > 1) {
            return false;
        }
        break;

    default:
        if (!config_set(CONFIG_ARCH_X86_GENERIC)) {
            return false;
        }
    }

    return true;
}

static BOOT_CODE bool_t
try_boot_sys(
    unsigned long multiboot_magic,
    multiboot_info_t* mbi
)
{
    /* ==== following code corresponds to the "select" in abstract specification ==== */

    acpi_rsdt_t* acpi_rsdt; /* physical address of ACPI root */
    paddr_t mods_end_paddr; /* physical address where boot modules end */
    paddr_t load_paddr;
    word_t i;
    p_region_t ui_p_regs;
    multiboot_module_t *modules = (multiboot_module_t*)(word_t)mbi->mod_list;

    if (multiboot_magic != MULTIBOOT_MAGIC) {
        printf("Boot loader not multiboot compliant\n");
        return false;
    }
    cmdline_parse((const char *)(word_t)mbi->cmdline, &cmdline_opt);

    if ((mbi->flags & MULTIBOOT_INFO_MEM_FLAG) == 0) {
        printf("Boot loader did not provide information about physical memory size\n");
        return false;
    }

    if (!x86_cpuid_initialize()) {
        printf("Warning: Your x86 CPU has an unsupported vendor, '%s'.\n"
               "\tYour setup may not be able to competently run seL4 as "
               "intended.\n"
               "\tCurrently supported x86 vendors are AMD and Intel.\n",
               x86_cpuid_get_identity()->vendor_string);
    }

    if (!is_compiled_for_microarchitecture()) {
        printf("Warning: Your kernel was not compiled for the current microarchitecture.\n");
    }

#if CONFIG_MAX_NUM_NODES > 1
    /* copy boot code for APs to lower memory to run in real mode */
    if (!copy_boot_code_aps(mbi->mem_lower)) {
        return false;
    }
    /* Initialize any kernel TLS */
    mode_init_tls(0);
#endif

    /* initialize the memory. We track two kinds of memory regions. Physical memory
     * that we will use for the kernel, and physical memory regions that we must
     * not give to the user. Memory regions that must not be given to the user
     * include all the physical memory in the kernel window, but also includes any
     * important or kernel devices. */
    boot_state.mem_p_regs.count = 0;
    init_allocated_p_regions();
    if (mbi->flags & MULTIBOOT_INFO_MMAP_FLAG) {
        if (!parse_mem_map(mbi->mmap_length, mbi->mmap_addr)) {
            return false;
        }
    } else {
        /* calculate memory the old way */
        p_region_t avail;
        avail.start = HIGHMEM_PADDR;
        avail.end = ROUND_DOWN(avail.start + (mbi->mem_upper << 10), PAGE_BITS);
        if (!add_mem_p_regs(avail)) {
            return false;
        }
    }

    boot_state.ki_p_reg.start = PADDR_LOAD;
    boot_state.ki_p_reg.end = kpptr_to_paddr(ki_end);

    /* copy VESA information from multiboot header */
    if ((mbi->flags & MULTIBOOT_INFO_GRAPHICS_FLAG) == 0) {
        boot_state.vbe_info.vbeMode = -1;
        printf("Multiboot gave us no video information\n");
    } else {
        boot_state.vbe_info.vbeInfoBlock = *mbi->vbe_control_info;
        boot_state.vbe_info.vbeModeInfoBlock = *mbi->vbe_mode_info;
        boot_state.vbe_info.vbeMode = mbi->vbe_mode;
        printf("Got VBE info in multiboot. Current video mode is %d\n", mbi->vbe_mode);
        boot_state.vbe_info.vbeInterfaceSeg = mbi->vbe_interface_seg;
        boot_state.vbe_info.vbeInterfaceOff = mbi->vbe_interface_off;
        boot_state.vbe_info.vbeInterfaceLen = mbi->vbe_interface_len;
    }

    printf("Kernel loaded to: start=0x%lx end=0x%lx size=0x%lx entry=0x%lx\n",
           boot_state.ki_p_reg.start,
           boot_state.ki_p_reg.end,
           boot_state.ki_p_reg.end - boot_state.ki_p_reg.start,
           (paddr_t)_start
          );

    /* remapping legacy IRQs to their correct vectors */
    pic_remap_irqs(IRQ_INT_OFFSET);
    if (config_set(CONFIG_IRQ_IOAPIC)) {
        /* Disable the PIC so that it does not generate any interrupts. We need to
         * do this *before* we initialize the apic */
        pic_disable();
    }

    /* get ACPI root table */
    acpi_rsdt = acpi_init();
    if (!acpi_rsdt) {
        return false;
    }

    /* check if kernel configuration matches platform requirments */
    if (!acpi_fadt_scan(acpi_rsdt)) {
        return false;
    }

    if (!config_set(CONFIG_IOMMU) || cmdline_opt.disable_iommu) {
        boot_state.num_drhu = 0;
    } else {
        /* query available IOMMUs from ACPI */
        acpi_dmar_scan(
            acpi_rsdt,
            boot_state.drhu_list,
            &boot_state.num_drhu,
            MAX_NUM_DRHU,
            &boot_state.rmrr_list
        );
    }

    /* query available CPUs from ACPI */
    boot_state.num_cpus = acpi_madt_scan(acpi_rsdt, boot_state.cpus, &boot_state.num_ioapic, boot_state.ioapic_paddr);
    if (boot_state.num_cpus == 0) {
        printf("No CPUs detected\n");
        return false;
    }

    if (config_set(CONFIG_IRQ_IOAPIC)) {
        if (boot_state.num_ioapic == 0) {
            printf("No IOAPICs detected\n");
            return false;
        }
    } else {
        if (boot_state.num_ioapic > 0) {
            printf("Detected %d IOAPICs, but configured to use PIC instead\n", boot_state.num_ioapic);
        }
    }

    if (!(mbi->flags & MULTIBOOT_INFO_MODS_FLAG)) {
        printf("Boot loader did not provide information about boot modules\n");
        return false;
    }

    printf("Detected %d boot module(s):\n", mbi->mod_count);

    if (mbi->mod_count < 1) {
        printf("Expect at least one boot module (containing a userland image)\n");
        return false;
    }

    mods_end_paddr = 0;

    for (i = 0; i < mbi->mod_count; i++) {
        printf(
            "  module #%ld: start=0x%x end=0x%x size=0x%x name='%s'\n",
            i,
            modules[i].start,
            modules[i].end,
            modules[i].end - modules[i].start,
            (char *) (long)modules[i].name
        );
        if ((sword_t)(modules[i].end - modules[i].start) <= 0) {
            printf("Invalid boot module size! Possible cause: boot module file not found by QEMU\n");
            return false;
        }
        if (mods_end_paddr < modules[i].end) {
            mods_end_paddr = modules[i].end;
        }
    }
    mods_end_paddr = ROUND_UP(mods_end_paddr, PAGE_BITS);
    assert(mods_end_paddr > boot_state.ki_p_reg.end);

    printf("ELF-loading userland images from boot modules:\n");
    load_paddr = mods_end_paddr;

    load_paddr = load_boot_module(modules, load_paddr);
    if (!load_paddr) {
        return false;
    }

    /* calculate final location of userland images */
    ui_p_regs.start = boot_state.ki_p_reg.end;
    ui_p_regs.end = ui_p_regs.start + load_paddr - mods_end_paddr;

    printf(
        "Moving loaded userland images to final location: from=0x%lx to=0x%lx size=0x%lx\n",
        mods_end_paddr,
        ui_p_regs.start,
        ui_p_regs.end - ui_p_regs.start
    );
    memcpy((void*)ui_p_regs.start, (void*)mods_end_paddr, ui_p_regs.end - ui_p_regs.start);

    /* adjust p_reg and pv_offset to final load address */
    boot_state.ui_info.p_reg.start -= mods_end_paddr - ui_p_regs.start;
    boot_state.ui_info.p_reg.end   -= mods_end_paddr - ui_p_regs.start;
    boot_state.ui_info.pv_offset   -= mods_end_paddr - ui_p_regs.start;

    /* ==== following code corresponds to abstract specification after "select" ==== */

    if (!platAddDevices()) {
        return false;
    }

    /* Total number of cores we intend to boot */
    ksNumCPUs = boot_state.num_cpus;

    printf("Starting node #0 with APIC ID %lu\n", boot_state.cpus[0]);
    if (!try_boot_sys_node(boot_state.cpus[0])) {
        return false;
    }

    if (config_set(CONFIG_IRQ_IOAPIC)) {
        ioapic_init(1, boot_state.cpus, boot_state.num_ioapic);
    }

    /* initialize BKL before booting up APs */
    SMP_COND_STATEMENT(clh_lock_init());
    SMP_COND_STATEMENT(start_boot_aps());

    /* grab BKL before leaving the kernel */
    NODE_LOCK;

    printf("Booting all finished, dropped to user space\n");

    return true;
}

BOOT_CODE VISIBLE void
boot_sys(
    unsigned long multiboot_magic,
    multiboot_info_t* mbi)
{
    bool_t result;
    result = try_boot_sys(multiboot_magic, mbi);

    if (!result) {
        fail("boot_sys failed for some reason :(\n");
    }

    ARCH_NODE_STATE(x86KScurInterrupt) = int_invalid;
    ARCH_NODE_STATE(x86KSPendingInterrupt) = int_invalid;

    NODE_STATE(ksCurTime) = getCurrentTime();
    NODE_STATE(ksConsumed) = 0;

    schedule();
    activateThread();
}

