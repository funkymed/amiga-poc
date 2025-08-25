# Projet Mandarine Amiga

## Architecture

```
assets       # all the assets images and mods
build        # adf build and exe
disk         # disk tool c and s to make the disk bootable
src
├── SDI      # the SDI lib
├── ptplayer # the ptplayer lib to play mod
└── main_simple.c # main source code
```

## Compile

Compile the exe

```
make simple
```

Compile the ADF with assets and exe

```
make simple
makd adf-simple
```

## Run

Launch your amiga and put the disk, or copy the files into your HDD and start simple_amiga 