# Remote-Shell
A simple remote shell server/client app we had to implement last year (December 2018) as part of the course "Operating Systems" at Harokopio University.

As of now the server can execue most simple commands like ls -l or wc but not more complicated like text editors like nano.
    ./client localhost 9999

    127.0.0.1:>ls -l

    total 104

    -rwxr-xr-x 1 rg rg  9396 Jan  2 21:39 a.out

    -rwxr-xr-x 1 rg rg 13872 Jan  3 11:51 c

    -rw-r--r-- 1 rg rg  2056 Jan  3 11:52 client.c

    -rw------- 1 rg rg    51 Jan  3 12:02 file111

    -rw------- 1 rg rg    55 Jan  3 12:05 out

    127.0.0.1:>
# It can also handle:
Up to one pipe (Someday I will make it support more than one pipes if I have the time)

    ./client localhost 9999
    
    127.0.0.1:>ls -l | wc -l

    9

 Redirections from and to files
 
    ./client localhost 9999

    127.0.0.1:>ls -l > data.txt

    data written to file

    127.0.0.1:>cat data.txt

    total 104

    -rwxr-xr-x 1 rg rg  9396 Jan  2 21:39 a.out

    -rwxr-xr-x 1 rg rg 13872 Jan  3 11:51 c

    -rw-r--r-- 1 rg rg  2056 Jan  3 11:52 client.c

    -rw------- 1 rg rg     0 Jan  3 15:06 data.txt

    -rw------- 1 rg rg    51 Jan  3 12:02 file111

    127.0.0.1:>
    
I will probably not be maintaining this project but maybe someone finds it usefull. 

You're free to use however you want the code in this project.  