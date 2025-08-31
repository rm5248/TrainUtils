logic-xml-diff
==============

Logical XML differ

First 0.1 version. Very rough edges.  
Runs in every OS X / Linux environment.

Requirements:  
xsltproc

Usage example
-------------

    ./logic-xml-diff.sh examples/1.xml examples/2.xml


Why?
----
This two XML files are the same logically.  
But attributes are not in the same order and additional spacing/newlines do not allow normal diff.

    <test><hello a="1" b="2/"><test/>
    
is the same as

    <test>
          <hello b="2" a="1"  />
      <test/>
      
logic-xml-diff puts everything in order and diffs the ordered XMLs.


Have fun!  
Marvin  
http://therealmarv.com
