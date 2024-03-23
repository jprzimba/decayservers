<?php
if(!defined('INITIALIZED'))
	exit;

class Vocations implements Iterator, Countable
{
	private $vocations = array();
	private $XML;
	public $iterator = 0;

	public function __construct($file)
	{
		$XML = new DOMDocument();
		if(!$XML->load($file))
			new Error_Critic('', 'Vocations::__construct - cannot load file <b>' . htmlspecialchars($file) . '</b>');

		$this->XML = $XML;
		$_tmp_vocations = array();

		foreach($XML->getElementsByTagName('vocation') as $vocation)
		{
			if($vocation->hasAttribute('id') && $vocation->hasAttribute('name'))
			{
				$vocationData = array();
				$vocationData['id'] = $vocation->getAttribute('id');
				$vocationData['name'] = $vocation->getAttribute('name');
				if($vocation->hasAttribute('manamultiplier'))
					$vocationData['manamultiplier'] = $vocation->getAttribute('manamultiplier');
				else
					$vocationData['manamultiplier'] = 1;
				if($vocation->hasAttribute('gainhp'))
					$vocationData['gainhp'] = $vocation->getAttribute('gainhp');
				else
					$vocationData['gainhp'] = 0;
				if($vocation->hasAttribute('gainmana'))
					$vocationData['gainmana'] = $vocation->getAttribute('gainmana');
				else
					$vocationData['gainmana'] = 0;
				if($vocation->hasAttribute('gaincap'))
					$vocationData['gaincap'] = $vocation->getAttribute('gaincap');
				else
					$vocationData['gaincap'] = 0;

				if($vocation->hasAttribute('gainhpticks'))
					$vocationData['gainhpticks'] = $vocation->getAttribute('gainhpticks');
				else
					$vocationData['gainhpticks'] = 1;
				if($vocation->hasAttribute('gainhpamount'))
					$vocationData['gainhpamount'] = $vocation->getAttribute('gainhpamount');
				else
					$vocationData['gainhpamount'] = 0;

				if($vocation->hasAttribute('gainmanaticks'))
					$vocationData['gainmanaticks'] = $vocation->getAttribute('gainmanaticks');
				else
					$vocationData['gainmanaticks'] = 1;
				if($vocation->hasAttribute('gainmanaamount'))
					$vocationData['gainmanaamount'] = $vocation->getAttribute('gainmanaamount');
				else
					$vocationData['gainmanaamount'] = 0;

				if($vocation->hasAttribute('gainsoulticks'))
					$vocationData['gainsoulticks'] = $vocation->getAttribute('gainsoulticks');
				else
					$vocationData['gainsoulticks'] = 1;

				if($vocation->hasAttribute('attackspeed'))
					$vocationData['attackspeed'] = $vocation->getAttribute('attackspeed');
				else
					$vocationData['attackspeed'] = 2000;

				$_tmp_vocations[$vocation->getAttribute('id')] = $vocationData;
			}
			else
				new Error_Critic('#C', 'Cannot load vocation. <b>id</b> or/and <b>name</b> parameter is missing');
		}
		foreach($_tmp_vocations as $_tmp_vocation)
		{
			$this->vocations[$_tmp_vocation['id']] = new Vocation($_tmp_vocation);
		}
	}
	/*
	 * Get vocation
	*/
	public function getVocation($id)
	{
		if(isset($this->vocations[$id]))
			return $this->vocations[$id];
		return false;
	}
	/*
	 * Get vocation name without getting vocation
	*/
	public function getVocationName($id)
	{
		if($vocs = self::getVocation($id))
			return $vocs->getName();
		return false;
	}

	#[\ReturnTypeWillChange]
	public function current()
	{
		return $this->vocations[$this->iterator];
	}	

	#[\ReturnTypeWillChange]
	public function rewind(): void
	{
		$this->iterator = 0;
	}	

	#[\ReturnTypeWillChange]
	public function next(): void
	{
		++$this->iterator;
	}	

	#[\ReturnTypeWillChange]
	public function key(): mixed
	{
		return $this->iterator;
	}	

	#[\ReturnTypeWillChange]
	public function valid(): bool
	{
		return isset($this->vocations[$this->iterator]);
	}	

	#[\ReturnTypeWillChange]
	public function count(): int
	{
		return count($this->vocations);
	}
	

}