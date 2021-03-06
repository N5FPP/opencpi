# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.


DIR=target-$OCPI_TOOL_HOST
PROG=$DIR/ad9361_config_proxy_test_app

for var in "$@"; do
  if [ "$var" == "verify" ]; then

    OCPI_LIBRARY_PATH=$OCPI_LIBRARY_PATH:../../devices/ad9361_config_proxy.rcc/:../fmcomms_2_3_rx.rcc/:../fmcomms_2_3_tx.rcc/:./assemblies/empty/container-empty_ml605_cfg_1rx_0tx_fmcomms_2_3_lpc_lvds_cnt_1rx_0tx_bypassasm_fmcomms_2_3_lpc_LVDS_ml605/:./assemblies/empty/container-empty_zed_cfg_1rx_0tx_fmcomms_2_3_lpc_lvds_cnt_1rx_0tx_bypassasm_fmcomms_2_3_lpc_LVDS_zed/ ./$PROG #$OUT

    X=$?
    if [ "$X" != "0" ]; then
      exit $X
    fi
  fi
done

exit 0
