# RAN Tester UE Components

This is a collection of attack tools for use in the RAN Tester UE system. Building in a docker container is recommended

---

See the our comprehensive [documentation ](https://docs.rantesterue.org) for more info on our attacks and metrics.

## Building Each Component

1. Ran Tester UE

```bash
mkdir -p build && cd build
cmake -DENABLE_RTUE=ON ..
```

2. SSB Spoofer

```bash
mkdir -p build && cd build
cmake -DENABLE_SSB_SPOOFER=ON ..
```

3. UU Agent

```bash
mkdir -p build && cd build
cmake -DENABLE_UU_AGENT=ON ..
```

## Using in the RAN Tester UE system

There are dockerfiles that build these binaries in the repo [https://github.com/oran-testing/ran-tester-ue](https://github.com/oran-testing/ran-tester-ue)

Specify the containers you want in the `build_spec` section of the config:

```yaml
build_spec:
  - component: uu-agent
		docker_image: ghcr.io/oran-testing/uu-agent
    pull: false
```
