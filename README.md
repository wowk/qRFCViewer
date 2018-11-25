qRFCView: a RFC viewer - Built on macOS and with Qt5 
==================================
Changes
--------
- Built under Qt5 (5.x) only (not compatible with Qt4) 
- HiDPI for icons (new icons added for modern look and feel, creditability for icons' authors is added to "About" menu)
- Default font is Courier

Please view the previous version at http://saghul.github.com/qrfcview-osx which build under Qt4

Build 
----------
- using CLI
```
    cd to_folder
    qmake
    make
```

- using Qt Creator
```
    import project then build all
```

License
--------------
GNU GPL v2 (see the license file) 


<br/>
<br/>

=====================================<br/><br/>
在原程序的基础上添加了翻译功能， 在阅读RFC的时候用鼠标选中单词或语句<br/>
便可显示其译文。现在支持的翻译 API 有百度翻译 和 Yandex翻译。Google<br/>
翻译 API 收费所以没有加入。<br/>
<br/>
通过 Edit-> Translator 可以配置翻译选项。<br/>