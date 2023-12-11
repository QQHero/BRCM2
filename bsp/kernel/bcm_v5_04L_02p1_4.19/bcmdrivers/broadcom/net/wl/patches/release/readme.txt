# This direcitory include all latest patches BRCM provoides based on the BSP version and 
# Wifi Version. The patches are automatically applied at the beginning of build process. 
#
#   ***** !!!! NOT SUGGESTED TO DO ANY CHANGES TO THE FILES UNDER THIS DIRECTORY !!!! ***
#   *****  !!! except the CUSTOM directory !!!                                        ***
#
#
# All patches are order in 00xx-anyword.patch,do not make any changes to it.  
#
# To make it more flexible, we have "customer" directory under both BSP and wifi directory for
# you to include your own patch here in case you have to do some changes beyond BRCM changes or
# some proposed changes during issue debugging session. The patches under the change directory 
# should take the same format as other chagnes to have 00xx-anywowrd.patch. These patches are
# always get patched after regulare patches are done. 
