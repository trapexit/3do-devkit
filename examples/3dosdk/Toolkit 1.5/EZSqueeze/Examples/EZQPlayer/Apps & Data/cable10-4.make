#  MPW script file for creating an EZSqueeze stream from a QuickTime Movie 
#  Starts the file Cable.dm, a 30 frame/second  15 second raw uncompressed movie
#
#	Track         ID     mediaType     Start Time     Time Scale     Duration     Type
#	    0      Movie          <NA>              0         600.00         9000     <NA>
#	    1     84A05C        "vide"              0          30.00          450   "raw "
#
#
#    Lets create a 10fps EZSqueeze stream at 4 bits/pixel  (4.6 bpp actually)
#        we will drop frames to go from 30fps to 15 fps     ( -k flag)
#     Use the -k flag to drop frames (instead of running 1/3 speed )
#     First create the EZSqueeze chunk file
#
#
#
       movietoezq -f 10 -k cable.dm
#   
#
#     This creates an EZQ Chunk file cable.dm.EZQF
#
#     Now run the weaver to weave this chunk file into a stream
#
#
#
		weaver -v -o cable10-4.stream < ccweave.script
#
#
#
#
#
#    Now lets delete the intermediate chunk file to save space
#
#
#
#		delete cable.dm.EZQF
#
#
#    Now move the resilting stream file to your remote directory and try it
#		duplicate cable10-4.stream {3DORemote}
#
#    	ezqplayer  cable10-4.stream
#
#   Don't forget to hit Option-Equal  to allow the mac enough cycles to process
#    the I/O requests from the 3DO