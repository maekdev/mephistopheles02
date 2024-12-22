# mephisto02
Mephisto Mondial II chess game emulator (revised version)

An emulator for the popular chess game, which allows you to play Mephisto again...
![MephRunning](https://raw.githubusercontent.com/maekdev/maekdev/main/media/mephistopheles02/mephX_run.gif)

## Table of Contents
- [Introduction](#Introduction)
- [Installation](#Installation)
- [Comment](#Comment)
- [License](#license)

## Introduction
This is a emulator for the [Mephisto Mondial II](https://www.schach-computer.info/wiki/index.php/Mephisto_Mondial_II), a chess computer out of the 90s era.

If you are interested in retro chess gaming, check out [Franz Huber’s CB-Emu compilation](https://fhub.jimdofree.com/) which is based on Mame and contains a lot of the old platforms - including the Mondial2 and plenty of ROM binaries.

I’ve started this project several years ago, just as proof-of-concept (and due to a mix of chess nostalgy & mental boredom during the pandemic situation). I've decided to rework the code and make it compilable and accessible, so in this branch you find the upgraded version of the code including many improvements you'd expect from chess programs these days.

During alpha version developent of this, I stumbled over the Mame version, but I found it very annoying when doing a wrong movement and the pieces just disappeared, so I was looking for a different way to do it: 
In my code I calculate the correct piece position by recapitulating all the previous movements from the initial board position. So it is always the latest state and you could also apply a custom board setup and calculate the next move.
This is unfortunately necessary, as the actual position is not stored directly in the memory and the other emulators rely on the player to move all pieces correctly.

## Installation

IMPORTANT: This code does not contain a ROM file. 
If you want to extract the ROM from real hardware, check out the legacy branch of this git, which contains AVR code + some instructions how to do this.
Otherwise, you can easily find some downloads in the web for the original file.

Windows: No installation required. Just get the mondial2 rom file and put it into the same folder as the executable.

Linux: just run make libs and make and make run (I might build an AUR package once for ARCH Linux someday)
'''bash
make libs
make
./bin/meph

## Comment

I’ve called the project „mephistopheles02: lucifer’s life“, because when analysing the ROM-file after extraction (see legacy-branch), I found some strange blocks inside including the letters which reminded me on LUCIFER’S LIFE and many whitespaces. This could be an incident or it could also be an easter egg though I didn’t further analyse/reverse engineer.
As for the hardware, I’ve extracted the firmware from Mephisto Mondial and Mondial2 – and especially for the latter one I have to say I’m quite impressed about the work of the original programmer [Frans Morsch](https://www.schach-computer.info/wiki/index.php/Morsch%2C_Frans): This guy managed to squeeze an opening dictionary and (for my player experience) a super powerful engine in just a few kB of memory & RAM. Quite impressive work...

## License

Published under MIT license (see LICENSE)
