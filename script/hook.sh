#! /bin/bash
#********************************************************************
# file: hook.sh                       date: Mon 2025-07-28 09:01:23 *
#                                                                   *
# Description:                                                      *
#                                                                   *
#                                                                   *
# Maintainer:  (yinxianglu)  <yinxianglu1993@gmail.com>             *
#                                                                   *
# This file is free software;                                       *
#   you are free to modify and/or redistribute it                   *
#   under the terms of the GNU General Public Licence (GPL).        *
#                                                                   *
# Last modified:                                                    *
#                                                                   *
# No warranty, no liability, use this at your own risk!             *
#*******************************************************************/



VMID="$1"
PHASE="$2"

case "$PHASE" in
	pre-start)
		;;
	post-start)
		killall -SIGUSR2 tbs
		;;
	pre-stop)
		;;
	post-stop)
		killall -SIGUSR2 tbs
		;;
	*)
		;;
esac

exit 0

#********************* End Of File: hook.sh *********************/
