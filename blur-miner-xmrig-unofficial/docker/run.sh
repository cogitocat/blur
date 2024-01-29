#!/usr/bin/env bash

POOL=$1
WALLET=$2
if [[ -z $POOL ]]; then
	POOL="cogitocat.strangled.net:52542"
fi
if [[ -z $WALLET ]]; then
	WALLET="bL3j7Pbbc49ZM3Rmx9XabReVRLTruCMZjNFa6wd59tZS9TAGwc2MsstFvtamTq6DSzKN12CRzaQ4ecQWHznJeycD3BFi6F3ap"
fi

cd /blur-miner/
cat << EOF > config.json
{
    "api": {
        "id": null,
        "worker-id": null
    },
    "http": {
        "enabled": false,
        "host": "127.0.0.1",
        "port": 0,
        "access-token": null,
        "restricted": true
    },
    "autosave": true,
    "background": false,
    "colors": true,
    "cpu": {
        "enabled": true,
        "huge-pages": true,
        "hw-aes": true,
        "priority": null,
        "memory-pool": false,
        "cn/blur": {
            "threads": null
        }
    },
    "opencl": {
        "enabled": false
    },
    "cuda": {
        "enabled": false,
        "loader": "xmrig-cuda.dll",
        "nvml": true,
        "cn/blur": [{
            "index": 0,
            "threads": 64,
            "blocks": 30,
            "bfactor": 0,
            "bsleep": 0,
            "affinity": -1
        }]
    },
    "log-file": null,
    "pools": [{
        "url": "$POOL",
        "user": "$WALLET",
        "enabled": true,
        "tls": false,
        "tls-fingerprint": null,
        "daemon": true,
        "daemon-poll-interval": 25
    }],
    "print-time": 5,
    "health-print-time": 60,
    "retries": 5,
    "retry-pause": 5,
    "syslog": true,
    "user-agent": null,
    "watch": true
}

EOF

#modprobe msr

#
#if cat /proc/cpuinfo | grep "AMD Ryzen" > /dev/null;
#then
#echo "Detected Ryzen"
#wrmsr -a 0xc0011022 0x510000
#wrmsr -a 0xc001102b 0x1808cc16
#wrmsr -a 0xc0011020 0
#wrmsr -a 0xc0011021 0x40
#echo "MSR register values for Ryzen applied"
#elif cat /proc/cpuinfo | grep "Intel" > /dev/null;
#then
#echo "Detected Intel - Disable tuning for now... cogitocat"
#echo wrmsr -a 0x1a4 0xf
#echo "MSR register values for Intel applied"
#else
#echo "No supported CPU detected"
#fi


MSR_FILE=/sys/module/msr/parameters/allow_writes

if test -e "$MSR_FILE"; then
        echo on > $MSR_FILE
else
        modprobe msr allow_writes=on
fi

if grep -E 'AMD Ryzen|AMD EPYC' /proc/cpuinfo > /dev/null;
        then
        if grep "cpu family[[:space:]]\{1,\}:[[:space:]]25" /proc/cpuinfo > /dev/null;
                then
                        if grep "model[[:space:]]\{1,\}:[[:space:]]97" /proc/cpuinfo > /dev/null;
                                then
                                        echo "Detected Zen4 CPU"
                                        wrmsr -a 0xc0011020 0x4400000000000
                                        wrmsr -a 0xc0011021 0x4000000000040
                                        wrmsr -a 0xc0011022 0x8680000401570000
                                        wrmsr -a 0xc001102b 0x2040cc10
                                        echo "MSR register values for Zen4 applied"
                                else
                                        echo "Detected Zen3 CPU"
                                        wrmsr -a 0xc0011020 0x4480000000000
                                        wrmsr -a 0xc0011021 0x1c000200000040
                                        wrmsr -a 0xc0011022 0xc000000401570000
                                        wrmsr -a 0xc001102b 0x2000cc10
                                        echo "MSR register values for Zen3 applied"
                                fi
                else
                        echo "Detected Zen1/Zen2 CPU"
                        wrmsr -a 0xc0011020 0
                        wrmsr -a 0xc0011021 0x40
                        wrmsr -a 0xc0011022 0x1510000
                        wrmsr -a 0xc001102b 0x2000cc16
                        echo "MSR register values for Zen1/Zen2 applied"
                fi
elif grep "Intel" /proc/cpuinfo > /dev/null;
        then
                echo "Detected Intel CPU"
                wrmsr -a 0x1a4 0xf
                echo "MSR register values for Intel applied"
else
        echo "No supported CPU detected"
fi

sysctl -w vm.nr_hugepages=1250
/blur-miner/xmrig
