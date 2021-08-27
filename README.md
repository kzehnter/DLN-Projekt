# DLN-Project

This project is for a class called Wireless Networks. The code is for an Arduino MKR1000 which is connected to an ePaper-Display by Waveshare.
The idea was to have a display mounted on a laboratory door which runs on a Lithium-Ion-Battery and gets updates via WiFi. 
The text on the display can be updated by sending messages to a Telegram-Bot. 
It allows different font sizes and when the end of a line is reached the text will jump to the next line.

The code is mainly c++, all files in the epd4in2 folder are written by Waveshare but have been debugged and adjusted for this usecase.
