/* -*- tab-width : 2 -*- */
#include "opt.h"
#include "cmd-install.h"
static int in_resume=0;
static char *flags =NULL;
struct install_impls *install_impl;

struct install_impls *impls_to_install[]={
  &impls_sbcl_bin,
};

extern int extract(const char *filename, int do_extract, int flags,const char* outputpath,Function2 f,void* p);

int installed_p(struct install_options* param) {
  int ret;
  char *i,*impl;

  impl=q(param->impl);
  //TBD for util.
  i=s_cat(configdir(),q("impls"),q(SLASH),q(param->arch),q(SLASH),q(param->os),q(SLASH),
          q(impl),q(param->version?SLASH:""),q(param->version?param->version:""),q(SLASH),NULL);
  ret=directory_exist_p(i);
  cond_printf(1,"directory_exist_p(%s)=%d\n",i,ret);
  s(i),s(impl);
  return ret;
}

int install_running_p(struct install_options* param) {
  /* TBD */
  return 0;
}

int start(struct install_options* param) {
  char *home=configdir(),*p;
  char *localprojects=cat(home,"local-projects/",NULL);
  setup_uid(1);
  ensure_directories_exist(localprojects);
  s(localprojects);
  if(installed_p(param)) {
    printf("%s/%s is already installed. Try (TBD) for the forced re-installation.\n",param->impl,param->version?param->version:"");
    return 0;
  }
  if(install_running_p(param)) {
    printf("It seems another installation process for $1/$2 is in progress somewhere in the system.\n");
    return 0;
  }
  p=cat(home,"tmp",SLASH,param->impl,param->version?"-":"",param->version?param->version:"",SLASH,NULL);
  ensure_directories_exist(p);
  s(p);

  p=cat(home,"tmp",SLASH,param->impl,param->version?"-":"",param->version?param->version:"",".lock",NULL);
  delete_at_exit(p);
  touch(p);

  s(p),s(home);
  return 1;
}

char* download_archive_name(struct install_options* param) {
  char* ret=cat(param->impl,param->version?"-":"",param->version?param->version:"",NULL);
  ret= param->arch_in_archive_name==0?s_cat(ret,q((*(install_impl->extention))(param)),NULL):
    s_cat(ret,cat("-",param->arch,"-",param->os,q((*(install_impl->extention))(param)),NULL),NULL);
  return ret;
}

int download(struct install_options* param) {
  char* home=configdir();
  char* url=install_impl->uri;
  char* archive_name=download_archive_name(param);
  char* impl_archive=cat(home,"archives",SLASH,archive_name,NULL);
  if(!file_exist_p(impl_archive)
     || get_opt("download.force",1)) {
    printf("Downloading %s\n",url);
    /*TBD proxy support... etc*/
    if(url) {
      ensure_directories_exist(impl_archive);
      int status = download_simple(url,impl_archive,0);
      if(status) {
        printf("Download Failed with status %d. See download_simple in src/download.c\n", status);
        return 0;
        /* fail */
      }
      s(url);
    }
  } else printf("Skip downloading %s\n",url);
  s(impl_archive),s(home);
  return 1;
}

DEF_SUBCMD(cmd_install) {
  char** argv=firstp(arg_);
  int argc=(int)rest(arg_);
  dealloc((void*)arg_);

  install_cmds *cmds=NULL;
  struct install_options param;
  quicklisp=1;
  param.os=uname();
  param.arch=uname_m();
  param.arch_in_archive_name=0;
  param.expand_path=NULL;
  if(argc!=1) {
    int ret=1,k;
    for(k=1;k<argc;++k) {
      int i,pos;
      param.impl=argv[k];
      pos=position_char("/",param.impl);
      if(pos!=-1) {
        param.version=subseq(param.impl,pos+1,0);
        param.impl=subseq(param.impl,0,pos);
      }else {
        param.version=NULL;
        param.impl=q(param.impl);
      }

      for(install_impl=NULL,i=0;i<sizeof(impls_to_install)/sizeof(struct install_impls*);++i) {
        struct install_impls* j=impls_to_install[i];
        if(strcmp(param.impl,j->name)==0) {
          install_impl=j;
        }
      }
      if(install_impl) { 
        for(cmds=install_impl->call;*cmds&&ret;++cmds)
          ret=(*cmds)(&param);
        if(ret) { // after install latest installed impl/version should be default for 'run'
          struct opts* opt=global_opt;
          struct opts** opts=&opt;
          char* home=configdir();
          char* path=cat(home,"config",NULL);
          char* v=cat(param.impl,".version",NULL);
          char* version=param.version;
          if(!install_impl->util) {
            int i;
            for(i=0;version[i]!='\0';++i)
              if(version[i]=='-')
                version[i]='\0';
            set_opt(opts,"default.lisp",param.impl);
            set_opt(opts,v,version);
            save_opts(path,opt);
          }
          s(home),s(path),s(v);
        }
      }
      {
        int i,j,argc_;
        char** tmp;
        char* install_ros=s_cat2(lispdir(),q("install.ros"));
        if(verbose&1) {
          fprintf(stderr,"%s is not implemented internal. %s argc:%d\n",param.impl,install_ros,argc);
          for(i=0;i<argc;++i)
            fprintf(stderr,"%s:",argv[i]);
          fprintf(stderr,"\n");
        }
        tmp=(char**)alloc(sizeof(char*)*(argc+9));
        i=0;
        tmp[i++]=q("--");
        tmp[i++]=install_ros;
        tmp[i++]=q("install");
        tmp[i++]=q(argv[1]);
        for(j=2;j<argc;tmp[i++]=q(argv[j++]));
        argc_=i;
        if(verbose&1) {
          int j;
          fprintf(stderr,"argc_=%d",argc_);
          for(j=0;j<argc_;++j)
            fprintf(stderr,"argv[%d]=%s,",j,tmp[j]);
        }
        for(i=0;i<argc_;i+=dispatch(argc_-i,&tmp[i],&top));
        for(j=0;j<argc_;s(tmp[j++]));
        dealloc(tmp);
        return 0;
      }
      if(param.version)s(param.version);
      s(param.impl),s(param.arch),s(param.os);
      s(param.expand_path);
      if(!ret)
        exit(EXIT_FAILURE);
    }
  }else {
    char* tmp[]={"help","install"};
    dispatch(2,tmp,&top);
    exit(EXIT_SUCCESS);
  }
  return 0;
}

LVal register_cmd_install(LVal a) {
  return add_command(a,"install"    ,NULL,cmd_install,1,1);
}
