#! /bin/bash

NGPWD=`realpath .`
mkdir -p $NGPWD/../out/nginx
NGPWD=$NGPWD/../out/nginx
SRCPATH="src"
BUILDOUTPATH="out"


# COMPILEPC=pc
#GITSERVER="github.com.cnpmjs.org"
GITSERVER="gitclone.com/github.com"

# CROSS_PREFIX=$(echo ${CROSS_COMPILE##*/})
# CROSS_PATH=$(echo ${CROSS_COMPILE%/*})
if [ "$COMPILEPC" == "pc" ]; then
	CROSS_COMPILE=
	CROSS_PATH=
else
	RUNHOST="arm-linux"
	if [ "$CROSS_COMPILE" == "" ]; then
		CROSS_PREFIX=arm-linux-gnueabihf-
	else
		CROSS_PREFIX=$(basename $CROSS_COMPILE)
	fi
	if [ "$CROSS_COMPILE" == "" ]; then
		CROSS_PATH=/home/vencol/code/gcc/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin
	else
		CROSS_PATH=$(dirname $CROSS_COMPILE)
	fi	
fi

export PATH=$CROSS_PATH:$PATH

if [ "$CROSS_PREFIX" == "" ]; then
	NGINX_BIN="pcnginx"
	PLAMBIT=64  #x86_64
else
	NGINX_BIN="nginx"
	PLAMBIT=32  #arm32
fi

getSrc(){
	cd $NGPWD/$SRCPATH
	TARFORM=tar.gz
	CLONEPATH=""
	if [ "$1" == "nginx" ]; then
		NGINX_VER="1.19.6"
		BRANCH=release-$NGINX_VER
		MODULEPATH=$1-$BRANCH
		CLONEPATH=https://$GITSERVER/$1/$1.git
		TARPATH=https://github.91chifun.workers.dev//https://github.com/$1/$1/archive/$BRANCH.$TARFORM
		NGINX_MODULE_PATH=$MODULEPATH
	elif [ "$1" == "nginx-http-flv-module" ]; then
		NGINX_HTTP_FLV_MODULE_VER="1.2.8"
		BRANCH=v$NGINX_HTTP_FLV_MODULE_VER
		MODULEPATH=$1-$NGINX_HTTP_FLV_MODULE_VER
		CLONEPATH=https://$GITSERVER/winshining/$1.git
		TARPATH=https://github.91chifun.workers.dev//https://github.com/winshining/$1/archive/$BRANCH.$TARFORM
		HTTP_FLV_MODULE_PATH=$MODULEPATH
	elif [ "$1" == "pcre" ]; then
		PCRE_VER="8.44"
		BRANCH=$1-$PCRE_VER
		MODULEPATH=$BRANCH
		TARPATH=https://ftp.pcre.org/pub/$1/$BRANCH.$TARFORM
		PCRE_MODULE_PATH=$MODULEPATH
	elif [ "$1" == "zlib" ]; then
		ZLIB_VER="1.2.11"
		BRANCH=$1-$ZLIB_VER
		MODULEPATH=$BRANCH
		TARPATH=https://zlib.net/$BRANCH.$TARFORM
		ZLIB_MODULE_PATH=$MODULEPATH
	elif [ "$1" == "openssl" ]; then
		OPENSSL_VER="3.0.0-alpha11"
		BRANCH=$1-$OPENSSL_VER
		MODULEPATH=$1-$BRANCH
		CLONEPATH=https://$GITSERVER/$1/$1.git
		TARPATH=https://github.91chifun.workers.dev//https://github.com/$1/$1/archive/$BRANCH.$TARFORM
		OPENSSL_MODULE_PATH=$MODULEPATH
	else
		echo "no config fou $1, pls add it in build.sh getSrc function"
		exit 1
	fi

	if [ ! -f "$BRANCH.$TARFORM" ]; then
		wget $TARPATH
		if [ $? -eq 0 ]; then
			tar -xf $BRANCH.$TARFORM 
		else
			exit 1
		fi
	else
		tar -xf $BRANCH.$TARFORM 
	fi

	if [ $? -ne 0 ]; then
		rm -r $SRCPATH $BRANCH.$TARFORM
		echo "tar $BRANCH.$TARFORM fail"
	fi
	
	if [ "$CLONEPATH" != "" ]; then
		if [ ! -d $MODULEPATH ]; then
			set -e
			git clone $CLONEPATH $MODULEPATH
			cd $MODULEPATH
			git checkout $BRANCH
			mkdir -p $BUILDOUTPATH
			set +e
		fi
	fi
	echo "have nginx src in $MODULEPATH for branch $BRANCH"
}

changeConfigForCrossGcc () {
	cd $NGPWD/$SRCPATH/$NGINX_MODULE_PATH
	# auto/cc/name
	# checking for C compiler ... found but is not working
	# ./configure: error: can not detect int size
	# 因为nginx编译的时候除了检查cc是否存在，还需要执行编译后的程序。很明显交叉编译的程序无法执行。
	sed -r -i "/ngx_feature_run=yes/ s/.*/\tngx_feature_run=no/g" auto/cc/name
	sed -r -i "/exit 1/ s/.*//1" auto/cc/name
	
	# auto/types/sizeof
	# checking for int size ...objs/autotest: 1: objs/autotest: Syntax error: word unexpected (expecting ")") bytes
	# ./configure: error: can not detect int size
	sed -r -i "/ngx_size=`$NGX_AUTOTEST`/ s/.*/\tngx_size=4/g" auto/types/sizeof
	# auto/optioons
	# configure: error: in `/home/src/pcre-8.42':
	# configure: error: C compiler cannot create executables
	if [ "$CROSS_PREFIX" == "" ]; then
		sed -r -i "/PCRE_CONF_OPT=/ s/.*/PCRE_CONF_OPT=/g" auto/options
	else
		sed -r -i "/PCRE_CONF_OPT=/ s/.*/PCRE_CONF_OPT=--host=${RUNHOST}/g" auto/options
	fi

	#auto/lib/openssl/make
	#.openssl/lib/libssl.a: error adding symbols: File format not recognized	
	sed -r -i "/ngx_prefix no-shared no-threads .*OPT/ s//ngx_prefix no-shared no-threads linux-generic$PLAMBIT \$OPENSSL_OPT/g" auto/lib/openssl/make
}

changeMakefileAfterConfig() {
	cd $NGPWD/$SRCPATH/$NGINX_MODULE_PATH

    # 屏蔽nginx 的configure信息
    CONHEAD_FILE=`find $BUILDOUTPATH -name "ngx_auto_config.h"`
    
	# DEL_LINE=`sed -n "/NGX_CONFIGURE/="  ${CONHEAD_FILE}`
    #     sed -i "${DEL_LINE}d" ${CONHEAD_FILE}
    # echo "#undef NGX_CONFIGURE " >> ${CONHEAD_FILE}
    # echo "#define NGX_CONFIGURE \"./configure\"" >> ${CONHEAD_FILE}
   
	 # src/os/unix/ngx_errno.c:37:31: error: NGX_SYS_NERR undeclared (first use in this function)
   	 # 因为交叉编译程序无法本地运行，导致NGX_SYS_NERR宏没有赋值。解决办法，手动编辑 objs/ngx_auto_config.h，加上	
     echo "#ifndef NGX_SYS_NERR" >> ${CONHEAD_FILE}
     echo "#define NGX_SYS_NERR 132" >> ${CONHEAD_FILE}
     echo "#endif" >> ${CONHEAD_FILE}
	 
	 # /home/src/nginx-1.14.0/src/core/ngx_cycle.c:476: undefined reference to 'ngx_shm_alloc'
	 # /home/src/nginx-1.14.0/src/core/ngx_cycle.c:685: undefined reference to 'ngx_shm_free'
     echo "#ifndef NGX_HAVE_SYSVSHM" >> ${CONHEAD_FILE}
     echo "#define NGX_HAVE_SYSVSHM 1" >> ${CONHEAD_FILE}
     echo "#endif" >> ${CONHEAD_FILE}

    # # 删除makefile 多余的几行

    #     DEL_LINE=`sed -n "/build\:/="  Makefile  | awk 'END {print}'`
    # # 因为是有 2 行，删除以后文件会发生变化
    #     sed -i "${DEL_LINE}d" Makefile
    #     sed -i "${DEL_LINE}d" Makefile

    #     DEL_LINE=`sed -n "/install\:/="  Makefile  | awk 'END {print}'`
    #     sed -i "${DEL_LINE}d" Makefile
    #     sed -i "${DEL_LINE}d" Makefile

    #     DEL_LINE=`sed -n "/modules\:/="  Makefile  | awk 'END {print}'`
    #     sed -i "${DEL_LINE}d" Makefile
    #     sed -i "${DEL_LINE}d" Makefile

}

setNginxCompileConfig (){
	#--user=nginx --group=nginx		set to use nginx for nginxuser but not root, for save
	# need to add user for nginx, sudo useradd -s /sbin/nologin -M nginx
	BASECONFIG="--user=nginx --group=nginx --with-debug  \
		--prefix= \
		--sbin-path=nginx \
		--builddir=$BUILDOUTPATH \
		--modules-path=modules \
		--pid-path=logs/nginx.pid \
		--lock-path=logs/nginx.lock \
		--http-log-path=logs/access.log \
		--error-log-path=logs/error.log \
		--http-scgi-temp-path=logs/temp-scgi \
		--http-uwsgi-temp-path=logs/temp-uwsgi \
		--http-proxy-temp-path=logs/temp-proxy \
		--http-fastcgi-temp-path=logs/temp-fastcgi \
		--http-client-body-temp-path=logs/temp-client-body \
		--with-ld-opt=-lpthread \
		--with-cc=${CROSS_PREFIX}gcc \
		--with-cc-opt=-Wno-error \
		--with-cpp=${CROSS_PREFIX}g++ "

	BASE_MODULE=" --with-http_realip_module \
	    --with-http_flv_module \
	    --with-http_mp4_module \
		--with-http_stub_status_module \
		--with-http_gzip_static_module "
		#--with-http_v2_module \
		#--with-http_addition_module \
		#--with-http_xslt_module \
		#--with-http_xslt_module=dynamic \
		#--with-http_image_filter_module \
		#--with-http_image_filter_module=dynamic \
		#--with-http_geoip_module \
		#--with-http_geoip_module=dynamic \
		#--with-http_sub_module \
		#--with-http_dav_module \
		#--with-http_auth_request_module \
		#--with-http_random_index_module \
	   	#--with-http_secure_link_module \
		#--with-http_degradation_module \
		#--with-http_slice_module \
		#--with-http_stub_status_module \
	
	STREAM_OPT="--with-stream \
	    --without-http_upstream_zone_module \
	    --without-stream_upstream_zone_module "
	
	#PCRE_OPT="--with-pcre-opt=--host=${RUNHOST}"
	PCRE_OPT=""
	CONFIGpcre="--with-pcre=$NGPWD/$SRCPATH/$PCRE_MODULE_PATH ${PCRE_OPT}"
	MODULE_ON_PCRE=""
	CONFIG_ON_PCRE="$CONFIGpcre $PCRE_OPT $MODULE_ON_PCRE"
	
	#ZLIB_OPT="--with-zlib-opt=--host=${RUNHOST}"
	ZLIB_OPT=""
	CONFIGzlib="--with-zlib=$NGPWD/$SRCPATH/$ZLIB_MODULE_PATH ${ZLIB_OPT}"
	MODULE_ON_ZLIB=""
	CONFIG_ON_ZLIB="$CONFIGzlib $ZLIB_OPT $MODULE_ON_ZLIB"
	
	#OPENSSL_OPT="--with-openssl-opt=os/compiler:${CROSS_PREFIX}gcc"
	OPENSSL_OPT="--with-openssl-opt=--cross-compile-prefix=${CROSS_PREFIX} "
	#OPENSSL_OPT=""
	CONFIGopenssl="--with-openssl=$NGPWD/$SRCPATH/$OPENSSL_MODULE_PATH ${OPENSSL_OPT}"
	MODULE_ON_OPENSSL=" --with-http_ssl_module "
	CONFIG_ON_OPENSSL="$CONFIGopenssl $OPENSSL_OPT $MODULE_ON_OPENSSL"
	
	ADD_MODULE="--add-module=$NGPWD/$SRCPATH/$HTTP_FLV_MODULE_PATH"
	
	if [ "$CROSS_PREFIX" == "" ]; then
		ALLCONFIG="$BASECONFIG $BASE_MODULE $STREAM_OPT $ADD_MODULE " 
	else
		ALLCONFIG="$BASECONFIG $BASE_MODULE $STREAM_OPT \
			$CONFIG_ON_ZLIB $CONFIG_ON_PCRE $CONFIG_ON_OPENSSL \
			$ADD_MODULE "
			#$CONFIG_ON_PCRE $CONFIG_ON_ZLIB $CONFIG_ON_OPENSSL"
	fi
	echo $ALLCONFIG
	#exit 1
	./auto/configure $ALLCONFIG
	#exit 1
}

configNgixServer (){
	echo "test tst"
}

paraseBuildOption (){
	cd  $NGPWD/$SRCPATH/$NGINX_MODULE_PATH
	if [ "$1" == "clean" ]; then
		make clean
		exit 1
	elif [ "$1" == "distclean" ]; then
		make distclean
		exit 1
	elif [ "$1" == "help" ]; then
		./auto/configure --help
		exit 1
	fi
}



mkdir -p $NGPWD/$SRCPATH
getSrc "pcre"
getSrc "zlib"
getSrc "openssl"
getSrc "nginx"
getSrc "nginx-http-flv-module"

set -e
paraseBuildOption $1
changeConfigForCrossGcc
echo "chaenge lajfla"
setNginxCompileConfig
changeMakefileAfterConfig
make -j8
make DESTDIR=$NGPWD/$NGINX_BIN install
#configNgixServer
set +e
echo "build nginx($NGINX_VER) with http-flv($NGINX_HTTP_FLV_MODULE_VER) successful. nginx path for $NGPWD/$NGINX_BIN"
exit 10

