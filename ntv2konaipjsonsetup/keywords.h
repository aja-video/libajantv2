#ifndef KEYWORDS_H
#define KEYWORDS_H

QString keywordList = " \n  \
                        2022-6 2022-2   2110 \n  \
protocol                                 x     2110 | 2022 \n  \
PTPMaster                                x     2110 PTP Master IP \n  \
4KMode                                   x     true | false \n  \
enable2022_7              x                    true | false \n  \
networkPathDifferential   x                    decimal number (milliseconds) \n  \
 \n  \
sfps \n  \
designator                x      x       x     linkA | linkB \n  \
ipAddress                 x      x       x     IP string \n  \
subnetMask                x      x       x     IP string \n  \
gateway                   x      x       x     IP string \n  \
 \n  \
receive \n  \
designator                x      x       x     channel1 | channel2 | channel3 | channel4 \n  \
stream                                   x     video | audio1 \n  \
sfp1SrcIPAddress          x      x       x     IP string \n  \
sfp1DestIPAddress         x      x       x     IP string \n  \
sfp1SrcPort               x      x       x     decimal number \n  \
sfp1DestPort              x      x       x     decimal number \n  \
sfp1Filter                x      x       x     hex number (vlan 0x01 | SrcIP 0x02 | DestIP 0x04 | SrcPort 0x08 | DestPort 0x10 | ssrc 0x20) \n  \
sfp1Enable                x      x             true | false \n  \
sfp2SrcIPAddress          x      x       x     IP string \n  \
sfp2DestIPAddress         x      x       x     IP string \n  \
sfp2SrcPort               x      x       x     decimal number \n  \
sfp2DestPort              x      x       x     decimal number \n  \
sfp2Filter                x      x       x     hex number (vlan 0x01 | SrcIP 0x02 | DestIP 0x04 | SrcPort 0x08 | DestPort 0x10 | ssrc 0x20) \n  \
sfp2Enable                x      x             true | false \n  \
playoutDelay              x      x       x     decimal number (milliseconds) \n  \
vlan                      x      x       x     decimal number \n  \
ssrc                      x      x       x     decimal number \n  \
payload                                  x     decimal number (payload type) \n  \
videoFormat                              x     format string see ntv2player -vl for a complete list (525i2997 | 625i25 | 720p5994 | 1080i5994 | 1080i50 | 1080p50)  \n  \
payloadLen                               x     decimal number (expert use only) \n  \
lastPayloadLen                           x     decimal number (expert use only) \n  \
pktsPerLine                              x     decimal number (expert use only) \n  \
enable                    x      x       x     true | false \n  \
 \n  \
transmit \n  \
designator                x      x       x     channel1 | channel2 | channel3 | channel4 \n  \
stream                                   x     video | audio 1 \n  \
sfp1RemoteIPAddress       x      x       x     IP string \n  \
sfp1RemotePort            x      x       x     decimal number \n  \
sfp1LocalPort             x      x       x     decimal number \n  \
sfp1Enable                x      x       x     true | false \n  \
sfp2RemoteIPAddress       x              x     IP string \n  \
sfp2RemotePort            x              x     IP string \n  \
sfp2LocalPort             x              x     decimal number \n  \
sfp2Enable                x      x       x     true | false \n  \
ssrc                      x      x       x     decimnal number \n  \
ttl                       x      x       x     decimal number \n  \
tos                       x      x       x     decimal number \n  \
payload                                  x     decimal number (payload type) \n  \
videoFormat                              x     format string see ntv2player -vl for a complete list (525i2997 | 625i25 | 720p5994 | 1080i5994 | 1080i50 | 1080p50)  \n  \
numAudioChannels                         x     decimal number 1-16 \n  \
firstAudioChannel                        x     decimal number 0-15 \n  \
audioPktInterval                         x     decimal number 125 or 1000 (microseconds) \n  \
enable                    x      x             true | false \n  \
";

#endif // KEYWORDS_H


