#!/bin/bash
#
#

ssh -n ubuntu@169.231.231.109 "kill -9 `ps uaxww | grep woofc | grep actions_runner | awk '{print $2}'`"

ssh -n ubuntu@169.231.231.109 'bash -c "cd /home/ubuntu/cspot/build/bin && nohup sh -c '\''setsid ./woofc-namespace-platform -b spawn > namespace.log 2>&1 < /dev/null &'\'' > /dev/null 2>&1 & disown"'


