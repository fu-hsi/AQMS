/*
SQLyog Community v12.5.1 (64 bit)
MySQL - 5.5.56-MariaDB : Database - admin_default
*********************************************************************
*/

/*!40101 SET NAMES utf8 */;

/*!40101 SET SQL_MODE=''*/;

/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;
/*Table structure for table `weather_pms` */

CREATE TABLE `weather_pms` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `dt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `pm1` smallint(6) DEFAULT NULL,
  `pm25` smallint(6) DEFAULT NULL,
  `pm10` smallint(6) DEFAULT NULL,
  `rv` smallint(6) DEFAULT NULL,
  `v` float(4,2) DEFAULT NULL,
  `temperature` float(4,2) DEFAULT NULL,
  `humidity` float(4,2) DEFAULT NULL,
  `pressure` float(6,2) DEFAULT NULL,
  `bm280_temp` float(4,2) DEFAULT NULL,
  `bm280_hum` float(4,2) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=146810 DEFAULT CHARSET=utf8;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;
