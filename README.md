# mephisto02
Mephisto Mondial II chess game emulator

A proof-of-concept project to play against this devil again...
![MephRunning](https://github.com/maekdev/maekdev/blob/main/mmedia/mephrunning.gif)

## Table of Contents
- [Introduction](#introduction)
- [NextStep](#NextStep)
- [Comment](#Comment)
- [How to extract firmware](#Firmware)
- [License](#license)
- [Contact](#contact)

## Introduction
This is a (proof-of-concept) emulator for the [Mephisto Mondial II](https://www.schach-computer.info/wiki/index.php/Mephisto_Mondial_II), a chess computer out of the 90s era.

If you want to just play chess with the platform from back then, you’re much better off using just [Franz Huber’s CB-Emu compilation](https://fhub.jimdofree.com/) which is based on Mame – and more mature than my code.

I’ve created this software, just as proof-of-concept (and due to a mix of chess nostalgy & mental boredom during the pandemic situation). For releasing the code I’ve removed the parts for which I was unsure regarding the copyrights, so you’ll only have the code I’ve created by myself, not including
- Mephisto Mondial2 ROM-file (I’ve downloaded this from real hardware bought on ebay)
- the 65c02-emulator code part (was downloaded from the web somewhere, but many good emulators exist as alterntive)
- the graphic art / picture files to display board + pieces (for the first version I’ve used the artwork from Mame)

From an educational perspective, it might though be interesting to look into my code and the analysis of the chess game memory structure, because other than the Mame based emulators, I’m using only the RAM to draw the board and piece position.
During alpha version developent of this, I stumbled over the Mame version, but I found it very annoying when doing a wrong movement and the pieces just disappeared, so I was looking for a different way to do it: 
In my code I calculate the correct piece position by recapitulating all the previous movements from the initial board position. So it is always the latest state and you could also apply a custom board setup and calculate the next move.
This is unfortunately necessary, as the actual position is not stored directly in the memory and the other emulators rely on the player to move all pieces correctly.

## NextStep

I don’t plan to rework the code. For me it’s just good enough for a quick game once in a while.
Maybe at a later step, I could bring it to another platform other than my linux machine. I would be even curious about the ELO strength of the eight levels of the Mondial2, so I was thinking of building a chess bot to find it out. 
Automatically moving the opponents piece once the calculation is finished might also be something easy to implement...

## Comment

I’ve called the project „mephistopheles02: lucifer’s life“, because when analysing the ROM-file after extraction, I found some strange blocks inside including the letters which reminded me on LUCIFER’S LIFE and many whitespaces. This could be an incident or it could also be an easter egg though I didn’t further analyse/reverse engineer.
As for the hardware, I’ve extracted the firmware from Mephisto Mondial and Mondial2 – and especially for the latter one I have to say I’m quite impressed about the work of the original programmer [Frans Morsch](https://www.schach-computer.info/wiki/index.php/Morsch%2C_Frans): This guy managed to squeeze an opening dictionary and (for my player experience) a super powerful engine in just a few kB of memory & RAM. Quite impressive work...

## Firmware
Howto extract the firmware / ROM:
For this I've opened the chess board, removed the EPROM (the one with the sticker on top) from the mainboard and connected it to an AVR to read out all data.
This is a picture of the original hardware
![Mephisto Mondial](https://github.com/maekdev/maekdev/blob/main/media/mephistopheles02/mondial.jpg)
And the circuitry inside
![Mondial opened up](https://github.com/maekdev/maekdev/blob/main/mmedia/mondialopen.jpg)
![Mondial PCB](https://github.com/maekdev/maekdev/blob/main/mmedia/mondialpcb.jpg)
Here you can see the read-out taking place
![MondialDownload](https://github.com/maekdev/maekdev/blob/main/mmedia/mondialdownload.gif)
