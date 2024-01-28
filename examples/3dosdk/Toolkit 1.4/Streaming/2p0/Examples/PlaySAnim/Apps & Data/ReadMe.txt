This folder contains sample data and scripts to demonstrate
the use of the animToSanm tool convert 3DO Anim files into
streamed animation format and to weave these streamed animation
files together into playable data streams.


Example 1:

Execute the MPW script 
 weavepool
 
 This script takes a 3DO animation file 'pool.anim'
 and turns it into two separate streamend animation
 files pool.chnk0 and pool.chnk2 using animToSanm. 
 Each file is given different frame rates and x,y positions.
 
 The weaver is then used to weave these two streamed 
 animation files together into a data stream 'pool.sanim'.
 The weave script pool.weave is used to control the
 parameters of the weaving process.   This also writes
 a stream header on the file describing the buffer sizes,
 number of buffer and types of subscribers used.  Playsanim
 looks at this header in order to properly initilize the 
 player.
 
 This file can be viewed by executing the program
   playsanim pool.sanim
   
 The original anim file pool.anim was created by concatenating
 10 cel frames together.  The frames were raytraced on a PC and
 converted using ppmto3do.
 
 
Example 2

Execute the MPW script 
  weavecount
  
 This script takes a 3DO animation file 'count.anim'
 and turns it into four separate streamend animation
 files count.chnk0 thru count.chnk3 using animToSanm. 
 Each file is given different frame rates and x,y positions.

 The weaver is then used to weave these four streamed 
 animation files together into a data stream 'count.sanim'.
 The weave script count.weave is used to control the
 parameters of the weaving process.   This also writes
 a stream header on the file describing the buffer sizes,
 number of buffer and types of subscribers used.  Playsanim
 looks at this header in order to properly initilize the 
 player.
 
 This file can be viewed by executing the program
   playsanim count.sanim
   
 The original anim file count.anim was created by 3DO Animator.
 Within Animator you can convert an animation file into a 3DO
 anim by 'Accessing the 3DO Palette'. You choose a cel format
 such as uncoded 16 or coded 8.  You then set the CCB values
 as appropriate to control packing, tranparency etc.
 You then select 'Rempap the Document' command in the 3DO palette
 window.  The resulting remapped document can now be saved as 
 a 3Do Anim file.
 
 
 