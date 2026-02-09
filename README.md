# AntGame

Well, it's a game about ants.

## What?

This is Antgame.

It's a game about ant nests trying to survive when pit against each other on a quest to stave off starvation the longest. They are placed on a two-dimensional map with walls and food. Their objective is to send forth their little ants to collect food and bring it back to them, all for a little more time until they collectively starve, as well as fight each other off to prevent each other from both getting food themselves, but also stealing food from other nests.

All of this means a LOT of ants per nest. More than two, at least. That's a lot more ants than I can control at once with one controller. Probably more than anyone can control, too. So that's not the exactly how this game is played.

No, instead, the gameplay of Antgame is controlling ants via a robotic queen which allows you to control your little tiny ants with nothing but code. You simply type away into whatever editor you choose, then later watch the chaos unfold, with absolutely no further intervention.

Basically, you make a bot which plays a game. 

## How?

In the background, there's a server. It handles letting all the players (bots) work at once. Don't worry about the server. What you have to worry about is your own client and your code. Of course, if you need to understand the game mechanics themselves, there's documentation for that (or at least there will be one day.) For your coding, though, you have a neat little python library to install, which will let you control your ants really easy. You can also read the documentation about the python library (which will exist one day) and the tutorials (which should be here eventually) if you're confused, though if I do my job well, you won't need to do it very much!

## Install

Well, it figures that to use software you have to install it. Here's how to install this one.

First, clone the repository
```
git clone https://github.com/chboo1/AntGame.git
```
or download it some other way. After that, if you don't want the python library, you can just compile everything, but it's very likely you do want the python library, which has an extra step.

First, install Python (specifically CPython, but that's what most people have anyways). Preferrably, Python 3.14, since that's the version the library was built with, but any 3.X should work as long as it's the same version you use to run the program after.

Anywho, you need to take the library files in your python installation and drop them off into the project so it can use the python files. There's two parts:

- An include path, which will have tons of files in .h, which is situated at 
  + `%localappdata%\Programs\Python\PythonXY\include` or `C:\Program Files\PythonXY\include` for Windows
  + `/Library/Frameworks/Python.framework/Versions/X.Y/include` or `/usr/local/bin/python3/include` for MacOS
  + `/usr/bin/python3/include` or `/usr/local/bin/python3/include` for Linux
- A library file, which will often look like libpythonX.Y.so / .pyd / .dylib, located in the same paths as above but replace the 'include' with 'lib'

Once that's done, you can compile everything. You will need some C++ compiler and make. In the main directory, run
```
make
```
or any equivalent for your system. If you need only the client library, run
```
make module
```
If you only need the server runtime, run
```
make server
```

Note: if the module fails to import on Windows, with the error "ImportError: DLL load failed while importing AntGame...", you might need to move the standard C++ libraries your version of g++ uses closer. That is, into either the same directory as the .pyd file or into your PATH. You can also call os.add_dll_directory with wherever those DLLs are BEFORE your 'import AntGame' statement.

## Roadmap

Things I need to do! It's quite complicated in reality, but in short, this is the game plan (mostly in chronological order):

- Cross-platform socket implementation (the most basic communication layer allows windows people and mac people and linux people to work together). Done!
- Server parts
	+ Communication layer (how the server talks to players and viewers, basically like your keyboard and screen, but over the internet). Done!
	+ Mechanics layer (how the game actually runs). Done!
	+ Website/Viewing experience (you can type in the address of the server and watch it all burn down! this is making sure that works fine). Done, but not very pretty!
- Client library (The thing that makes it easy for you. I doubt any of you want to learn how to code sockets.) Almost done!
- Fine tuning, playtesting and balancing. (Oh boy. This is when I fix my game after you rip it open with a chainsaw. Thank you, by the way.)
- \[Optional\] Map maker program. This is an optional addition that would let one easily create new maps. Of course, it's not strictly needed, but if I have additional time it's definitely happening!

- Known specific TODOs:
	+ Make all the antcs into unsigned shorts instead of unsigned chars - Not sure if I should do this one. 256 ants seems like a nice constraint to have.
	+ Add more ant types
	+ Add ^C support for windows
	+ Test deterministic-ness (Oh god no)
	+ Add more info to the running player list
	+ Implement nest stats
