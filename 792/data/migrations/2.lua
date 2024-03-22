function onUpdateDatabase()
	db.query("ALTER TABLE `players` ADD `balance` BIGINT UNSIGNED NOT NULL DEFAULT 0")
	return true
end
