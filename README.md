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

## Roadmap

Things I need to do! It's quite complicated in reality, but in short, this is the game plan (mostly in chronological order):

- Cross-platform socket implementation (the most basic communication layer allows windows people and mac people and linux people to work together). Done!
- Server parts
	+ Communication layer (how the server talks to players and viewers, basically like your keyboard and screen, but over the internet). Pretty well started.
	+ Mechanics layer (how the game actually runs).
	+ Website/Viewing experience (you can type in the address of the server and watch it all burn down! this is making sure that works fine).
- Client library (The thing that makes it easy for you. I doubt any of you want to learn how to code sockets.)
- Fine tuning, playtesting and balancing. (Oh boy. This is when I fix my game after you rip it open with a chainsaw. Thank you, by the way.)
- [Optional] Map maker program. This is an optional addition that would let one easily create new maps. Of course, it's not strictly needed, but if I have additional time it's definitely happening!
