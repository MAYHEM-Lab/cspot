curl -X POST -F "fileToUpload=@/home/smartfarm/zoe-pic.jpg" http://128.111.45.111/upload.php
curl-put.sh woof://128.111.45.83:57850/mnt/monitor/sedgtomayhem.bw 128.111.45.111 ./zoe-pic.jpg

yum -y install php php-common php-opcache php-mcrypt php-cli php-gd php-curl php-mysql

set max sizes in /etc/php.ini
post_max_size = 8000M
upload_tmp_dir = /tmp
upload_max_filesize = 8000M

put index.html in /var/www/html
put uploads.php /var/www/html
mkdir /var/www/html/uploads

chown everything to apache
restart httpd
