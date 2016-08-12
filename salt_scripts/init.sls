https://github.com/handong32/hoardthreadtest.git:
  git.latest:
    - target: /tmp/hoardthreadtest
    - user: root

hoardthreadtest-build:
  cmd.run:
    - cwd: /tmp/hoardthreadtest
    - name: |
        make -j Release || exit -1
        make threadtest || exit -1
    - env:
      - EBBRT_SRCDIR: '/tmp/ebbrt'
      - EBBRT_SYSROOT: '/tmp/ebbrt/toolchain/sysroot'
    - timeout: 300
    - require:
      - git: https://github.com/handong32/hoardthreadtest.git

