#!/bin/bash - 
#===============================================================================
#
#          FILE: parse_pcap.sh
# 
#         USAGE: ./parse_pcap.sh 
# 
#   DESCRIPTION: 
# 
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Lixing Song (), lsong2@nd.edu
#  ORGANIZATION: University of Notre Dame
#       CREATED: 08/18/2016 14:05
#      REVISION:  ---
#===============================================================================

set -o nounset                              # Treat unset variables as an error
filename=$1

if [ -s $filename ]; then
        tshark -r $filename -Y "wlan.fc.type != 2" -E separator="|" -T fields -e frame.time_epoch -e frame.time_delta_displayed -e frame.len -e wlan.ta -e wlan.ra -e wlan.fc.type_subtype -e wlan.fc.ds -e wlan.fc.retry -e radiotap.dbm_antsignal -e radiotap.channel.freq -e radiotap.datarate -e wlan.duration -e wlan.seq  -e radiotap.ampdu.flags.lastknown -e radiotap.ampdu.flags.last -e radiotap.ampdu.reference -e wlan_mgt.fixed.sequence -e wlan.ba.bm
fi

