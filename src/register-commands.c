#include "opt.h"

extern void register_cmd_run(void);
extern LVal register_cmd_install(LVal);
extern void register_cmd_internal(void);

#define OPT_SETVAL(sym,rexp)                          \
  DEF_SUBCMD(opt_##sym) {                             \
    char** argv=firstp(arg_);                         \
    int argc=(int)rest(arg_);                         \
    dealloc((void*)arg_);                             \
                                                      \
    sym=rexp;                                         \
    cond_printf(1,"opt:%s:%d\n",cmd->name,sym);       \
    return 1;}

OPT_SETVAL(verbose,(strcmp(cmd->name,"verbose")==0)?1|verbose<<1:verbose>>1)
OPT_SETVAL(testing,1+testing)
OPT_SETVAL(rc,(strcmp(cmd->name,"rc")==0)?1:0)
OPT_SETVAL(quicklisp,(strcmp(cmd->name,"quicklisp")==0)?1:0)

DEF_SUBCMD(opt_program0) {
  char** argv=firstp(arg_);
  int argc=(int)rest(arg_);
  dealloc((void*)arg_);

  if(cmd->name) {
    char* current=get_opt("program",0);
    current=cat(current?current:"","(:",cmd->name,")",NULL);
    set_opt(&local_opt,"program",current);
  }
  return 1;
}

DEF_SUBCMD(opt_take1) {
  char** argv=firstp(arg_);
  int argc=(int)rest(arg_);
  dealloc((void*)arg_);

  const char* arg=cmd->name;
  if(arg && argc>1)
    set_opt(&local_opt,arg,argv[1]);
  return 2;
}

#define OPT_APPEND(sym)                                            \
  DEF_SUBCMD(opt_##sym) {                                          \
    char** argv=firstp(arg_);                                      \
    int argc=(int)rest(arg_);                                      \
    dealloc((void*)arg_);                                          \
                                                                   \
    if(cmd->name && argc>1) {                                      \
      char* current=get_opt(#sym,0);                               \
      current=s_cat(current?q(current):q(""),                      \
                    q("(:"),q(cmd->name),q(" \""),                 \
                    escape_string(argv[1]),q("\")"),NULL);         \
      set_opt(&local_opt,#sym,current);                            \
    }                                                              \
    return 2;                                                      \
  }

OPT_APPEND(program)
OPT_APPEND(restart)
OPT_APPEND(final)

LVal register_runtime_options() {
  LVal opt=0;
  opt=add_command(opt,"version" ,NULL,opt_version,1,1);
  opt=add_command(opt,"wrap","-w",opt_take1,1,0);
  opt=add_command(opt,"image","-m",opt_take1,1,0);
  opt=add_command(opt,"lisp","-L",opt_take1,1,0);

  /*opt=add_command(opt,"file","-f",opt_program,1,0,"include lisp FILE while building","FILE");*/
  opt=add_command(opt,"load","-l",opt_program,1,0);
  opt=add_command(opt,"source-registry","-S",opt_program,1,0);
  opt=add_command(opt,"system","-s",opt_program,1,0);
  opt=add_command(opt,"load-system",NULL,opt_program,1,0);
  opt=add_command(opt,"package","-p",opt_program,1,0);
  opt=add_command(opt,"system-package","-sp",opt_program,1,0);
  opt=add_command(opt,"eval","-e",opt_program,1,0);
  opt=add_command(opt,"require",NULL,opt_program,1,0);
  opt=add_command(opt,"quit","-q",opt_program0,1,0);

  opt=add_command(opt,"restart","-r",opt_restart,1,0);
  opt=add_command(opt,"entry","-E",opt_restart,1,0);
  opt=add_command(opt,"init","-i",opt_restart,1,0);
  opt=add_command(opt,"print","-ip",opt_restart,1,0);
  opt=add_command(opt,"write","-iw",opt_restart,1,0);

  opt=add_command(opt,"final","-F",opt_final,1,0);

  opt=add_command(opt,"rc","-R",opt_rc,1,0);
  opt=add_command(opt,"no-rc","+R",opt_rc,1,0);
  opt=add_command(opt,"quicklisp","-Q",opt_quicklisp,1,0);
  opt=add_command(opt,"no-quicklisp","+Q",opt_quicklisp,1,0);
  opt=add_command(opt,"verbose","-v",opt_verbose,1,0);
  opt=add_command(opt,"quiet",NULL,opt_verbose,1,0);
  opt=add_command(opt,"test",NULL,opt_testing,1,0);
  opt=add_command(opt,"stdin",NULL,opt_program,0,0);
  return nreverse(opt);
}

LVal register_runtime_commands(struct proc_opt* top_) {
  top_->command=register_cmd_install(top_->command);
  top_->command=add_command(top_->command,"roswell-internal-use",NULL,cmd_internal,0,1);
  register_cmd_internal();
  register_cmd_run();
  return nreverse(top_->command);
}

void register_top(struct proc_opt* top_) {
  dispatch_init(top_);
  top_->option=register_runtime_options();
  top_->command=register_runtime_commands(top_);
  top_->top=(LVal)top_;
}
