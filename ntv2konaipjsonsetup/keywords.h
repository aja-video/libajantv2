#ifndef KEYWORDS_H
#define KEYWORDS_H

QString keywordList = " \n  \
                        2022-6 2022-2   2110 \n  \
Protocol                                 x      2110 | 2022 \n  \
Enable2022_7              x                     true | false \n  \
networkPathDifferential   x                     decimal number (milliseconds) \n  \
 \n  \
sfps \n  \
designator                x      x       x      top | bottom \n  \
IPAddress                 x      x       x      IP string \n  \
SubnetMask                x      x       x      IP string \n  \
Router                    x      x       x      IP string \n  \
 \n  \
receive \n  \
designator                x      x        x     channel1 | channel2 | channel3 | channel4 \n  \
stream                                    x     video | audio1 \n  \
LinkAEnable               x      x              true | false \n  \
primarySrcPort            x      x        x     decimal number \n  \
primarySrcIPAddress       x      x        x     IP string \n  \
primaryDestPort           x      x        x     decimal number \n  \
primaryDestIPAddress      x      x        x     IP string \n  \
primaryFilter             x      x        x     hex number (vlan 0x01 | SrcIP 0x02 | DestIP 0x04 | SrcPort 0x08 | DestPort 0x10 | ssrc 0x20) \n  \
LinkBEnable               x      x              true | false \n  \
secondarySrcPort          x                     decimal number \n  \
secondarySrcIPAddress     x                     IP string \n  \
secondaryDestPort         x                     decimal number \n  \
secondaryDestIPAddress    x                     IP string \n  \
secondaryFilter           x                     hex number (vlan 0x01 | SrcIP 0x02 | DestIP 0x04 | SrcPort 0x08 | DestPort 0x10 | ssrc 0x20) \n  \
playoutDelay              x      x        x     decimal number (milliseconds) \n  \
vlan                      x      x        x     decimal number \n  \
ssrc                      x      x        x     decimal number \n  \
payload                                   x     decimal number (payload type) \n  \
videoFormat                               x     format string (525i | 625i | 720p | 1080i | 1080i50 | 1080p | 1080p50) \
payloadLen                                x     decimal number (expert use only) \n  \
lastPayloadLen                            x     decimal number (expert use only) \n  \
pktsPerLine                               x     decimal number (expert use only) \n  \
Enable                    x      x        x     true | false \n  \
 \n  \
transmit \n  \
designator                x      x        x     channel1 | channel2 | channel3 | channel4 \n  \
stream                                    x     video | audio 1 \n  \
LinkAEnable               x      x        x     true | false \n  \
primaryLocalPort          x      x        x     decimal number \n  \
primaryRemoteIPAddress    x      x        x     IP string \n  \
primaryRemotePort         x      x        x     decimal number \n  \
LinkBEnable               x      x        x     true | false \n  \
secondaryLocalPort        x                     decimal number \n  \
secondaryRemoteIPAddress  x                     IP string \n  \
secondaryRemotePort       x                     IP string \n  \
ssrc                      x      x        x     decimnal number \n  \
ttl                       x      x        x     decimal number \n  \
tos                       x      x        x     decimal number \n  \
payload                                   x     decimal number (payload type) \n  \
videoFormat                               x     format string (e.g. NTV2_FORMAT_1080i_5994) \n  \
numAudioChannels                          *     decimal number 1-16 \n \
firstAudioChannel                         *     decimal number 0-15 \n \
audioPktInterval                          *     decimal number 125 or 1000 (microseconds) \n \
Enable                    x      x              true | false \n  \
";

#endif // KEYWORDS_H


