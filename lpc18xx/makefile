######################################################################## 
# $Id:: makefile 59 2007-11-02 23:21:55Z durgeshp                      $
# 
# Project: Standard compile makefile
# 
# Description: 
#  Makefile
# 
######################################################################## 
# Software that is described herein is for illustrative purposes only  
# which provides customers with programming information regarding the  
# products. This software is supplied "AS IS" without any warranties.  
# NXP Semiconductors assumes no responsibility or liability for the 
# use of the software, conveys no license or title under any patent, 
# copyright, or mask work right to the product. NXP Semiconductors 
# reserves the right to make changes in the software without 
# notification. NXP Semiconductors also make no representation or 
# warranty that such application will be suitable for the specified 
# use without further testing or modification. 
########################################################################


########################################################################
#
# Pick up the configuration file in make section
#
########################################################################
include ./makesection/makeconfig 

########################################################################
#
# Create a list of codebases to be compiled 
#
########################################################################

TARGETS		=fwlib 
TARGETS_CLN	=fwlib_clean 

########################################################################
#
# Pick up the default build rules 
#
########################################################################

include $(PROJ_ROOT)/makesection/makerule/$(DEVICE)/make.$(DEVICE).$(TOOL)

########################################################################
#
# Build the CSP and GEN libraries 
#
########################################################################

libs: $(TARGETS)

abl: fwlib

fwlib: 
	$(ECHO) "Building" $(FWLIB) "support package source ->" \
	$(FWLIB_SRC_DIR)
	@$(MAKE) TOOL=$(TOOL) -C $(FWLIB_SRC_DIR)
	$(ECHO) "done"

########################################################################
#
# Clean the CSP and GEN libraries 
#
########################################################################

clean: $(TARGETS_CLN)

abl_clean: fwlib_clean

fwlib_clean: 
	$(ECHO) "Cleaning" $(FWLIB) "support package ->" $(FWLIB_SRC_DIR)
	@$(MAKE) TOOL=$(TOOL) -C $(FWLIB_SRC_DIR) clean -s


# delete all targets this Makefile can make and all built libraries
# linked in
realclean: clean
	@$(MAKE) TOOL=$(TOOL) -C $(FWLIB_SRC_DIR) realclean

distclean: realclean
	@$(RMREC) "*.o"
	@$(RMREC) "*.elf"
	@$(RMREC) "*.exe"
	@$(RMREC) "*~"
	@$(RMREC) "*.map"
	@$(RMREC) "*.lst"
	@$(RMREC) "*.axf"
	@$(RMREC) "*.bin"
	@$(RMREC) "*.dbo"
	@$(RMREC) "*.dla"
	@$(RMREC) "*.dnm"
	@$(RMREC) "*.srec"
	@$(RMREC) ".depend"
	@$(RMREC) "*.wrn"

.PHONY: libs abl fwlib clean abl_clean fwlib_clean realclean distclean
