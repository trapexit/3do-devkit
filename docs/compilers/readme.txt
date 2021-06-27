
                         C++ 1.11 Release Notes
                         ----------------------


This CD contains ARM's C++ Toolkit version 1.11.

It is designed to work with ARM's Software Development Toolkit (SDT)
version 2.51.

C++ version 1.11 is functionally equivalent to version 1.10, but
contains a number of bug fixes, and introduces support for Windows
2000 and Solaris 2.7.

This release of C++ is supported on Windows 95 and 98, 2000, Windows
NT 4, Solaris 2.5.1, 2.6 and 2.7, HP/UX-10.

Full details of the changes introduced between C++ 1.01 and C++
1.10/1.11 can be found in Chapter 1 of the SDT User Guide and the SDT
Reference Guide, which are also supplied in PDF form on the SDT 2.51
CD.

Please refer to the Software Development Toolkit FAQ for the latest
information on the toolkit, which can be found on ARM's Web site at:

http://www.arm.com/DevSupp/Sales+Support/faq.html


Additional information is provided in Application notes which can be
found on ARM's Web site at:

http://www.arm.com/Documentation/AppNotes


General information of ARM's Development Systems can be found on ARM's
Web site at:

http://www.arm.com/DevSupp



The major bugs fixed in this release are as follows, although C++ also
benefits from many of bugs which have been fixed in the SDT 2.51
release, so you should read the SDT 2.51 readme file too:

1. C++ internal error unless extra "this->" scope resolution is added
    [Defect 25927]

2. C++ error "dw_2_writeinfo 7d != 7f" when compiling static data members in templates
    [Defect 26989]

