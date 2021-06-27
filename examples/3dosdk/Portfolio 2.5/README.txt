IMPORTANT: Portfolio 2.5 Examples Folder Release Notes

Please read the following information regarding the Portfolio 2.5 Examples
folder.

Makefiles
---------
All Example makefiles follow these guidelines:

* Each example will have or share an Apps_Data folder which contains the
  executable and data files used by that example.  When an example is
  rebuilt using the provided makefiles, the executables will be moved in
  to the Apps_Data folder.
  
* The makefile will not duplicate the example or its data files to the
  remote directory.
  
* All moves use the -y option so that no confirmation boxes appear when
  overwriting preexisting files.
  
* Some makefiles will not properly handle spaces or quotation marks in
  path names.  This includes the BuildExamples script located at the
  top level of the Examples folder.

Example Programs
----------------

* The EventBroker examples bs_example and joystick_example initialize the
  screen after the first event has arrived from the control port.  Once an
  event has arrived, the screen will be drawn properly.
  
* When running the example jsinteractivemusic, the file jsplaybgndmusic
  needs to be placed in the folder JSData.  If the file is not there, the
  program will complain with the message, "Can't spawn task for SoundHandler".

* The example jsplaybgndmusic looks in the current directory for data files
  it uses.  This file should be placed to the JSData folder as suggested in
  the previous bullet item.

* When running the example fontlibexample, the data file sample.font needs
  to be renamed to example.font or the example will not run.

