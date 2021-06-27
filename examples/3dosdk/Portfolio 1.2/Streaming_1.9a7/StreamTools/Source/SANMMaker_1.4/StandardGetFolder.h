/*******************************************************************************
* StandardGetFolder.h                                                         *
*                                                                              *
*    This little chunk o' code implements a way to let the user choose a       *
*    folder to save files in via a StandardFile Dialog.                        *
*                                                                              *
*    Since the code uses the CustomGetFile function and depends on the use of  *
*    FSSpec records, it only works under System 7.0 or later.                  *                        
*                                                                              *
*    And don't forget to include the custom dialog resources ( a 'DITL' and    *
*   'DLOG') in your project.                                                   *                                                                   *
*                                                                              *
*    Portions of this code were originally provided by Paul Forrester          *
*    (paulf@apple.com) to the think-c internet mailing list in response to my  *
*    my question on how to do exactly what this code does.  I've added a       *
*    couple of features, such as the ability to handle aliased folders and     *
*    the programmer definable prompt.  I also cleaned and tightened the code,  *
*    stomped a couple of bugs, and packaged it up neatly.  Bunches of work,    *
*    but I learned A LOT about Standard File, the File Manager, the Dialog     *
*    Manager, and the Alias Manager.  I tried to include in the comments some  *
*    of the neat stuff I discovered in my hours of pouring over Inside Mac.    *
*    Hope you find it educational as well as useful.                           *
*******************************************************************************/
#define rGetFolderDialog            2008

void StandardGetFolder (    Point               where,
                            Str255              message,
                            StandardFileReply   *mySFReply);