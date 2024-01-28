#  MPW script file for creating an EZSqueeze stream from a QuickTime Movie 
#  Starts the file Copter.dm, a 24 frame/second  30 second raw uncompressed movie
#
#	Track         ID     mediaType     Start Time     Time Scale     Duration     Type
#	    0      Movie          <NA>              0         600.00        18000     <NA>
#	    1     7F7A88        "vide"              0          24.00          720   "raw "
#
#
#    Lets create a 12fps EZSqueeze stream at 4 bits/pixel  (4.6 bpp actually) 
#        we will drop frames to go from 24fps to 12 fps     ( -k flag)
#     First create the EZSqueeze chunk file
#
#
#
       movietoezq -f 12 -k    copter.dm
#   
#
#     This creates an EZQ Chunk file cable.dm.EZQF
#
#     Now run the weaver to weave this chunk file into a stream
#
#
#
		weaver -v -o copter.stream < copter.script
#
#
#
#
#
#    Now lets delete the intermediate chunk file to save space
#
#
#
		delete copter.dm.EZQF
#
#
#    Now move the resilting stream file to your remote directory and try it
#		duplicate copter.stream {3DORemote}
#
#    	ezqplayer  copter.stream
#
#   Don't forget to hit Option-Equal  to allow the mac enough cycles to process
#    the I/O requests from the 3DO