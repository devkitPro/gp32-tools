#Iam README.TXT

The programm zda_compressor allows you to create
a container with compressed files in it.

The compressed data is stored without any header,
so the compressed data is very short, also no
uncompressor on Earth support the zda format, so 
it is not possible to uncompress the data on normal
Windows PCs. ( Winzip,Winrar,... )
UPDATE:
I extened this version, so it now outputs a zda file
with and without a header.


Usage:

./zda_compressor file1 file2 file3 

Compresses file1-3 and put the compressed data
to the File: data.zda


The screen output shows some information:

./zcontainer README.TXT zcontainer.c
filename,normal size,crunched size,offset
README.TXT,242,164,0
zcontainer.c,1205,543,164


How to read the Output ?

README.TXT,242,164,0
The File README.TXT is 242 Bytes long.
The compressed lenght is 164 Bytes.
The Startposition in the data.zda file is:0

zcontainer.c,1205,543,164
The File zcontainer.c is 1205 Bytes long.
The compressed lenght is 543 Bytes.
The Startposition in the data.zda file is:164


FILE: data.zda
|------README.TXT------|------zcontainer.c------|
0                     164                     164+543=707





!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  UPDATE UPDATE UPDATE UPDATE UPDATE UPDATE UPDATE UPDATE UPDATE UPDATE
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

This new version V1.0 can now create a new zda container format, with
header support. So you no longer need to know where your compressed data 
is stored in the file. 
