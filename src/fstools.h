#ifndef FSTOOLS_H
#define FSTOOLS_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>



class Fstools {
    public:
    static String errMsg;
    static String webListRoot(){
        String r = F("<strong>--- Root Directory: </strong><br>");

        File root = SPIFFS.open("/");
        if(!root){
            errMsg=F("Failed to open directory");
            return "";
        }
        File file = root.openNextFile();
        int fCount=0;
        while(file){
            r += (file.name());
            r += F("   SIZE: ");
            r += file.size();
            r += F(" byte  ->  <a href='/fs/read?name=");
            r += file.name();
            r += F("'>read</a>  ->  <a href='/fs/delete?name=");
            r += file.name();
            r += F("' onclick=\"return confirm('Are you sure?')\">delete</a> <br>");
            file = root.openNextFile();
            fCount++;
            }
        r += "\nTotal:"+String(fCount) + "<br>";
        r += F("<hr><br><a href='/'>Back to root</a>") ;
    return r;
        
    }
    static String fileToString(String fileName) {
        String result = "";
        File file = SPIFFS.open(fileName,"r");
        if (!file) return "I was unable to read file: " + fileName;
        while (file.available()){
            result += char(file.read());
        }
        file.close();
        return result;
    }
};

String Fstools::errMsg = "";

#endif