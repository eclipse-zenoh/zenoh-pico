/*
 * Minimal board PHY driver table for custom board configuration.
 *
 * When example.syscfg has customBoardEnable = true, SysConfig skips generating
 * the gEnetPhyDrvTbl symbol.  The enet-cpsw library references it externally,
 * so we provide an empty table here.  No PHY auto-negotiation is attempted;
 * the link is assumed up (useful for direct-attach / loopback / SGMII setups).
 */

#include <stddef.h>
#include <networking/enet/core/src/phy/enetphy_priv.h>

const EnetPhy_DrvInfoTbl gEnetPhyDrvTbl = {
    .numHandles  = 0U,
    .hPhyDrvList = NULL,
};
