#Possible combinations of commands
$  strace ls
$  ltrace ls
$ strace -e open ls
$ strace -e trace=open,read ls /home
$ strace -o output.txt ls
$ cat output.txt 
$ strace ./script.py | less  ~/.config/test
$ ps -C firefox-bin
$  sudo strace -p 1725 -o firefox_trace.txt
$ tail -f firefox_trace.txt
$ strace -t -e open ls /home  
#system calls related to automation files or java
$ strace -r ls   
/var/log/syslog
$ strace -c ls /home  
#ltrace touch file1.txt
#memory-log info programs
#ltrace -p [process-id]
#ltrace tar -hcf test.tar.x *tx  


sudo ltrace -p 47168
pip-tuberia number mario bross
ltrace -r cat > new.txt
ltrace -t pwd
ltrace -S ls
ltrace -o demo1.txt -S ls
cat demo1.txt
ltrace -e [chain-rules]
wc -l contacts.csv
head -15 contacts.csv
head -50 contacts.csv | tail -25 | tail-12 | head -6 | head -3 |./import.py --server test  

#Some solutions for users that might have CPU memory problems and the server is running slow, that is why testing files logs is important
#The reason is because in the app product, the requirements are not related with the test design and automation testing, the issues are always 
#related with the user memory problems, video recorders, internet speed and CPU memory corruption

Pytest
(Pdb) p ('./setup.py, head, tail')
help
python3 -m pdb app.py arg1 arg2

Python libraries request,urlib

ruby commands

#videos make the server slow
#Big images or the server processes
#Bots - reboting the suggestions
#anuncios y bots video converter
# user-CPU
locate static/001.webm
server/deploy_test_videos/
ls -l
deploy.sh <- file
grep ffmpeg *
#script runs daeomnize
nano ffmpeg
#solution to kill processes web servers 
for pid in $(pidof ffmpeg); do while kill -CONT $pid; do sleep 1; done; done
