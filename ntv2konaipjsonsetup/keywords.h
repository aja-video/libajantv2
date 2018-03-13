#ifndef KEYWORDS_H
#define KEYWORDS_H

QString keywordList = " \n  \
                        2022-6 2022-2   2110 \n  \
Protocol                                 x      2110 | 2022 \n  \
enable2022_7              x                     true | false \n  \
networkPathDifferential   x                     decimal number (milliseconds) \n  \
4KMode                                   x      true | false \n \
 \n  \
sfps \n  \
designator                x      x       x      linkA | linkB \n  \
ipAddress                 x      x       x      IP string \n  \
subnetMask                x      x       x      IP string \n  \
gateway                   x      x       x      IP string \n  \
 \n  \
receive \n  \
designator                x      x        x     channel1 | channel2 | channel3 | channel4 \n  \
stream                                    x     video | audio1 \n  \
linkAEnable               x      x              true | false \n  \
linkASrcPort              x      x        x     decimal number \n  \
linkASrcIPAddress         x      x        x     IP string \n  \
linkADestPort             x      x        x     decimal number \n  \
linkADestIPAddress        x      x        x     IP string \n  \
linkAFilter               x      x        x     hex number (vlan 0x01 | SrcIP 0x02 | DestIP 0x04 | SrcPort 0x08 | DestPort 0x10 | ssrc 0x20) \n  \
linkBEnable               x      x              true | false \n  \
linkBSrcPort              x               x     decimal number \n  \
linkBSrcIPAddress         x               x     IP string \n  \
linkBDestPort             x               x     decimal number \n  \
linkBDestIPAddress        x               x     IP string \n  \
linkBFilter               x               x     hex number (vlan 0x01 | SrcIP 0x02 | DestIP 0x04 | SrcPort 0x08 | DestPort 0x10 | ssrc 0x20) \n  \
playoutDelay              x      x        x     decimal number (milliseconds) \n  \
vlan                      x      x        x     decimal number \n  \
ssrc                      x      x        x     decimal number \n  \
payload                                   x     decimal number (payload type) \n  \
videoFormat                               x     format string (525i | 625i | 720p | 1080i | 1080i50 | 1080p | 1080p50) \
payloadLen                                x     decimal number (expert use only) \n  \
lastPayloadLen                            x     decimal number (expert use only) \n  \
pktsPerLine                               x     decimal number (expert use only) \n  \
enable                    x      x        x     true | false \n  \
 \n  \
transmit \n  \
designator                x      x        x     channel1 | channel2 | channel3 | channel4 \n  \
stream                                    x     video | audio 1 \n  \
linkAEnable               x      x        x     true | false \n  \
linkALocalPort            x      x        x     decimal number \n  \
linkARemoteIPAddress      x      x        x     IP string \n  \
linkARemotePort           x      x        x     decimal number \n  \
linkBEnable               x      x        x     true | false \n  \
linkBLocalPort            x               x     decimal number \n  \
linkBRemoteIPAddress      x               x     IP string \n  \
linkBRemotePort           x               x     IP string \n  \
ssrc                      x      x        x     decimnal number \n  \
ttl                       x      x        x     decimal number \n  \
tos                       x      x        x     decimal number \n  \
payload                                   x     decimal number (payload type) \n  \
videoFormat                               x     format string (e.g. NTV2_FORMAT_1080i_5994) \n  \
numAudioChannels                          *     decimal number 1-16 \n \
firstAudioChannel                         *     decimal number 0-15 \n \
audioPktInterval                          *     decimal number 125 or 1000 (microseconds) \n \
enable                    x      x              true | false \n  \
";

#endif // KEYWORDS_H


