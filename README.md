GoHttp
======
GoHttp is a simple web server written in C for educational purposes. This web server runs on GNU/Linux.

##What is implemented?
This web server is far from complete and the purpose of it is to be very light weight to give an idea of where to start when wanting to understand web servers and C.

It supports GET and HEAD so you can use it to receive any files that correspond with the mime types in mime.types.

##I want to add support for POST, can I contribute?
Sure! If you want to send a pull request please do! Keep in mind though that this is for educational purposes so keep the code clean and understandable - no golfing!

#How do I run it?

1. Download the source
2. Compile the source using GCC
3. Run

##Command line arguments
You can start the web server with the following command line arguments:

	-p port number
	-d run as daemon
	-l log file

##What about configuration?
You can open httpd.conf and change the following:

	wwwroot /home/frw/public_html/
	port 7000

#Credit
If it weren't for the course in Advance UNIX Programming that I took at Blekinge Institute of Technology I would never have written this. It all originated from a question on [StackOverflow](http://stackoverflow.com/questions/409087/creating-a-web-server-in-pure-c) from 2009 where I asked for information on how to write a simple web server in C.