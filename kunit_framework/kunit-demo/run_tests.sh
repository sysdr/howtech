#!/bin/bash
cd /kernel/linux-6.6
./tools/testing/kunit/kunit.py run --arch=um --kunitconfig=drivers/kunit_demo
