#!/usr/bin/python3

# This script is by @rspeir on GitHub: 
# https://github.com/krzmaz/pico-w-webserver-example/pull/1/files/4b3e78351dd236f213da9bebbb20df690d470476#diff-e675c4a367e382db6f9ba61833a58c62029d8c71c3156a9f238b612b69de279d
# Renamed output to avoid linking incorrect file

import os
import binascii

#Create file to write output into
output = open('htmldata.c', 'w') 

#Traverse directory, generate list of files
files = list()
os.chdir('./html_files')
for(dirpath, dirnames, filenames) in os.walk('.'):
    files += [os.path.join(dirpath, file) for file in filenames]

filenames = list()
varnames  = list()

#Generate appropriate HTTP headers
for file in files:

    if '404' in file:
        header = "HTTP/1.0 404 File not found\r\n"
    else:
        header = "HTTP/1.0 200 OK\r\n"

    header += "Server: lwIP/pre-0.6 (http://www.sics.se/~adam/lwip/)\r\n"

    if '.html' in file:
        header += "Content-type: text/html\r\n"
    elif '.shtml' in file:
        header += "Content-type: text/html\r\n"
    elif '.jpg' in file:
        header += "Content-type: image/jpeg\r\n"
    elif '.gif' in file:
        header += "Content-type: image/gif\r\n"
    elif '.png' in file:
        header += "Content-type: image/png\r\n"
    elif '.class' in file:
       header += "Content-type: application/octet-stream\r\n"
    elif '.js' in file:
       header += "Content-type: text/javascript\r\n"
    elif '.css' in file:
       header += "Content-type: text/css\r\n"
    elif '.svg' in file:
       header += "Content-type: image/svg+xml\r\n"
    else:
        header += "Content-type: text/plain\r\n"

    header += "\r\n"

    fvar = file[1:]                 #remove leading dot in filename
    fvar = fvar.replace('/', '_')   #replace *nix path separator with underscore
    fvar = fvar.replace('\\', '_')  #replace DOS path separator with underscore
    fvar = fvar.replace('.', '_')   #replace file extension dot with underscore

    output.write("static const unsigned char data{}[] = {{\n".format(fvar))
    output.write("\t/* {} */\n\t".format(file))

    #first set of hex data encodes the filename
    b = bytes(file[1:].replace('\\', '/'), 'utf-8')     #change DOS path separator to forward slash     
    for byte in binascii.hexlify(b, b' ', 1).split():
        output.write("0x{}, ".format(byte.decode()))
    output.write("0,\n\t")

    #second set of hex data is the HTTP header/mime type we generated above
    b = bytes(header, 'utf-8')
    count = 0
    for byte in binascii.hexlify(b, b' ', 1).split():
        output.write("0x{}, ".format(byte.decode()))
        count = count + 1
        if(count == 10):
            output.write("\n\t")
            count = 0
    output.write("\n\t")

    #finally, dump raw hex data from files
    with open(file, 'rb') as f:
        count = 0
        while(byte := f.read(1)):
            byte = binascii.hexlify(byte)
            output.write("0x{}, ".format(byte.decode()))
            count = count + 1
            if(count == 10):
                output.write("\n\t")
                count = 0
        output.write("};\n\n")

    filenames.append(file[1:])
    varnames.append(fvar)

for i in range(len(filenames)):
    prevfile = "NULL"
    if(i > 0):
        prevfile = "file" + varnames[i-1]

    output.write("const struct fsdata_file file{0}[] = {{{{ {1}, data{2}, ".format(varnames[i], prevfile, varnames[i]))
    output.write("data{} + {}, ".format(varnames[i], len(filenames[i]) + 1))
    output.write("sizeof(data{}) - {}, ".format(varnames[i], len(filenames[i]) + 1))
    output.write("FS_FILE_FLAGS_HEADER_INCLUDED | FS_FILE_FLAGS_HEADER_PERSISTENT}};\n")

output.write("\n#define FS_ROOT file{}\n".format(varnames[-1])) 
output.write("#define FS_NUMFILES {}\n".format(len(filenames)))