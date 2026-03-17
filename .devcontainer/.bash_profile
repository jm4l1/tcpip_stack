if [ "$PS1" ]; then

    if [ -x /usr/bin/tput ]; then
      if [ "x`tput kbs`" != "x" ]; then # We can't do this with "dumb" terminal
        stty erase `tput kbs`
      elif [ -x /usr/bin/wc ]; then
        if [ "`tput kbs|wc -c `" -gt 0 ]; then # We can't do this with "dumb" terminal
          stty erase `tput kbs`
        fi
      fi
    fi
   case $TERM in
    xterm*)
        if [ -e /etc/sysconfig/bash-prompt-xterm ]; then
		 	PROMPT_COMMAND=/etc/sysconfig/bash-prompt-xterm
		else
            PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME%%.*}:${PWD/#$HOME/~}\007"'
        fi
        ;;
    screen)
        if [ -e /etc/sysconfig/bash-prompt-screen ]; then
            PROMPT_COMMAND=/etc/sysconfig/bash-prompt-screen
        else
        PROMPT_COMMAND='echo -ne "\033_${USER}@${HOSTNAME%%.*}:${PWD/#$HOME/~}\033\\"'
        fi
        ;;
    *)
        [ -e /etc/sysconfig/bash-prompt-default ] && PROMPT_COMMAND=/etc/sysconfig/bash-prompt-default

        ;;
    esac
fi

stty erase '^?'
get_git_branch(){
	git branch 2> /dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/\1/'
}
parse_git_branch(){
  git branch 2> /dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/{\1/'
}
parse_git_head_tag(){
  git log  --oneline 2> /dev/null | head -n 1 | awk '{ print $1 }'
}
get_git_modified(){
  git status | grep '^\smodified' | awk '{print $2}'
}
get_git_deleted(){
  git status | grep '^\sdeleted' | awk '{print $2}'
}
show_git_info(){
		echo $(git branch 2> /dev/null | sed -e '/^[^*]/d' -e 's/* \(.*\)/{\1:/')$(git log --oneline 2> /dev/null |head -n 1 | sed -e 's/ .*/}/g')
}
PS1="\[\033[0;34m\][\u@\h:\[\033[0;32m\]\W\[\033[0;34m\]]\[\033[31m\]\$(show_git_info)\[\033[34m\]$\[\033[0m\]"

set -o noclobber

# 2.2) Listing, directories, and motion
alias ll="ls -alrtFhG"
alias la="ls -A"
alias ..='cd ..'
alias ...='cd ..;cd ..'

#alias ll='ls -altrh'
alias cp='cp -i'
alias mv='mv -i'
alias rm='rm -i'
export CLICOLOR=1

export PATH
export HISTSIZE=10000
export HISTFILESIZE=10000


#git
export PATH="/usr/local/git/bin:$PATH"


