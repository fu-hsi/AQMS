<?php
/**
 * ws.php - Web Service
 * Copyright (c) 2018 fu-hsi
 * MIT License
 */

error_reporting(0);
ini_set('display_errors', 0);
date_default_timezone_set('Europe/Warsaw');

// define('FU_PROFILE', true);

if (defined('FU_PROFILE')) {
    require_once 'config.php';
} else {
    define('WS_TOKEN', 'YOUR TOKEN');

    define('DB_HOST', 'YOUR DB HOST');
    define('DB_USER', 'YOUR DB USER');
    define('DB_PASS', 'YOUR DB PASS');
    define('DB_NAME', 'YOUR DB NAME');
}

class DB
{
    private static $db = null;
    private function __construct()
    {}
    public static function getConnection()
    {
        if (self::$db == null) {
            $dsn = sprintf('mysql:dbname=%s;host=%s', DB_NAME, DB_HOST);
            $options = array(
                PDO::MYSQL_ATTR_INIT_COMMAND => "SET NAMES utf8",
                PDO::ATTR_ERRMODE => PDO::ERRMODE_SILENT,
                PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            );
            self::$db = new PDO($dsn, DB_USER, DB_PASS, $options);
        }
        return self::$db;
    }
}

if (!empty($_SERVER['HTTP_X_TOKEN']) && $_SERVER['HTTP_X_TOKEN'] == WS_TOKEN) {

    // Allowed fields
    $whiteList = [
        'pm1', 'pm25', 'pm10',
        'rv', 'v',
        'temperature', 'humidity', 'pressure', 'bm280_temp', 'bm280_hum'
    ];

    // Clear array
    $whiteList = array_fill_keys($whiteList, null);

    // Prepare array
    $storeData = [];
    $storeData = array_merge($storeData, $whiteList, array_intersect_key($_POST, $whiteList));

    try {
        $db = DB::getConnection();
        $stm = $db->prepare('INSERT INTO weather_pms SET dt = NOW(), ' . implode(',', array_map(function ($key) {
            return $key . ' = :' . $key;
        }, array_keys($storeData))));
        $stm->execute($storeData);
    } catch (PDOException $e) {
        exit('DB error');
    }

}
?>