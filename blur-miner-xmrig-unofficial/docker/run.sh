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

#
if cat /proc/cpuinfo | grep "AMD Ryzen" > /dev/null; then
	echo "Detected Ryzen"
	wrmsr -a 0xc0011022 0x510000
	wrmsr -a 0xc001102b 0x1808cc16
	wrmsr -a 0xc0011020 0
	wrmsr -a 0xc0011021 0x40
	echo "MSR register values for Ryzen applied"
elif cat /proc/cpuinfo | grep "Intel" > /dev/null; then
	echo "Detected Intel - Disable tuning for now... ppasika"
	echo wrmsr -a 0x1a4 0xf
	echo "MSR register values for Intel applied"
else
	echo "No supported CPU detected"
fi

sysctl -w vm.nr_hugepages=1250
/blur-miner/xmrig
