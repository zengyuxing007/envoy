DROP TABLE IF EXISTS `person`;

CREATE TABLE `person` (

  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,

  `fname` varchar(32) NOT NULL DEFAULT '""',

  `lname` varchar(32) NOT NULL DEFAULT '""',

  `age` tinyint(3) NOT NULL DEFAULT '18',

  `sex` varchar(16) NOT NULL DEFAULT 'MALE',

  `income` int(11) NOT NULL DEFAULT '10000',

  PRIMARY KEY (`id`)

) ENGINE=InnoDB DEFAULT CHARSET=utf8;


LOCK TABLES `person` WRITE;

/*!40000 ALTER TABLE `person` DISABLE KEYS */;


INSERT INTO `person` (`id`, `fname`, `lname`, `age`, `sex`, `income`)

VALUES

	(1,'bill','gates',18,'MALE',88888888),

	(2,'vifoggy','huang',18,'FEMALE',888888);



/*!40000 ALTER TABLE `person` ENABLE KEYS */;

UNLOCK TABLES;
