# AntGame

Well, it's a game about ants.

## What?

This is Antgame.

It's a game about ant nests trying to survive when pit against each other on a quest to stave off starvation the longest.

This means a lot of ants. More than two, at least. That's a lot more ants than I can control at once with one controller. So that's not the point of this game.

No, instead, the goal of Antgame is to control ants via a robotic queen which allows you to control your little tiny ants with nothing but code. 

## How?

In the background, there's a server. Don't worry about the server. What you have to worry about is your own client and your code. Of course, if you need to understand the server, there's documentation for that (or at least there will be one day. TODO.) For that, you have a neat little python library to install and that's it. You can type `python3 antgamebot.py` and run it. You can also read the documentation about the python library (which will exist one day) and the tutorials (which should be here eventually) if you're confused.

## What's TODO?

Things I need to do! It's quite nitty-gritty, but in short, this is the game plan:

- Cross-platform socket implementation (the most basic communication layer). Done!
- Server
	+ Communication layer. In good progress.
	+ Mechanics layer.
	+ Website/Viewing experience.
	+ Fine-tuning.
- Client library
