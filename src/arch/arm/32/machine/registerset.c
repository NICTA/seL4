/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <arch/machine/registerset.h>

const register_t msgRegisters[] = {
    R2, R3, R4, R5
};

const register_t frameRegisters[] = {
    FaultInstruction, SP, CPSR,
    R0, R1, R8, R9, R10, R11, R12
};

const register_t gpRegisters[] = {
    R2, R3, R4, R5, R6, R7, R14
};

word_t getNBSendRecvDest(void)
{
    return getRegister(NODE_STATE(ksCurThread), nbsendRecvDest);
}
