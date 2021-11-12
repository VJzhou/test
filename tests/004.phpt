--TEST--
test_scale Basic test
--SKIPIF--
<?php
    if (!extension_load("test")) {
        echo 'skip';
    }
?>
--FILE--
<?php
    var_dump(test_scale(2.0));
    var_dump(test_scale(3.0));
?>
--EXPECT--
float(4)
float(6)