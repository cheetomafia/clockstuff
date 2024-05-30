just notes to self:

the clock was programmed to run off the light board, which means, yes it's written in the lovely c++, but pulls in dmx so it can communicate properly (yay :))

anywho, the way to actually run the clock on the board basically consist of the following after doing some wackass division
each time on the clock is mapped to a specific intensity from 0-100, with the clock running on ch 500

the *most* important four settings to remember are these:
0 --> sets the clock to idle (this won't do anything if the clock is currently in the middle of doing something/changing times)
100/full --> hard stop (this will stop the clock in place, no matter what it's doing)
97 --> reset the hour hand
99 --> reset the minute hand

the rest of the times start at 01 and go up by two for each 15 minute interval on the clock
ex:
01 --> 12pm/am
03 --> 12:15
goes up by "8" for every hour
33 --> 4pm/am
35 --> 4:15
37 --> 4:30 
39 --> 4:45
because of rounding issues with division in the code, 5 pm/am ended up beinng equal to "40", this was the only weird number we encountered
following this,
43 --> 5:15 (if 5 had been 41 as expected...)
56 --> 7

so on so forth, a whole chart was written up at one point but i don't have it anymore and i'm not doing the math for this - but you get the idea

the mechanical part of the clock doesn't really matter for whoever is reading this but for my own knowledge, this code is running two motors with encoders on them off an ardunio/motor controller combo - you have pictures and the thing still in your house just go look at it
