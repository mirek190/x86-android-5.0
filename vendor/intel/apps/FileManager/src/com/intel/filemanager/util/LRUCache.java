package com.intel.filemanager.util;

import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.Map;

public class LRUCache<K,V> {

	private static final float cacheLoadFactor = 0.75f;

	private LinkedHashMap<K,V> map;
	private int cacheSize;

	private static LRUCache instance = null;

	public static synchronized LRUCache getInstanceOfLRUCache(int cacheSize){
		if(null == instance){
			instance = new LRUCache(cacheSize);
		}
		return instance;
	}

	/**
	 * other class cannot new Cache directly 
	 * @param cacheSize
	 */
	private LRUCache (int cacheSize){
		this.cacheSize = cacheSize;
		int cacheCapacity = (int)Math.ceil(cacheSize/cacheLoadFactor) + 1;

		map = new LinkedHashMap<K,V>(cacheCapacity, cacheLoadFactor, true){
			private static final long serialVersionID = 1;

			protected boolean removeEldestEntry (Map.Entry<K,V> eldest){
				return size() > LRUCache.this.cacheSize;
			}
		};

	}

	/**
	 * return a value from cache
	 * @param key
	 * @return
	 */
	public synchronized V get (K key){
		return map.get(key);
	}


	/**
	 * put an object into the cache
	 * @param key
	 * @param value
	 */
	public synchronized void put (K key, V value){
		map.put(key, value);
	}

	/**
	 * clear the cache
	 */
	public synchronized void clear(){
		map.clear();
	}

	/**
	 * return the object number of the cache
	 * @return
	 */
	public synchronized int usedEntries(){
		return map.size();
	}

	/**
	 * return a set with copy of the cache content. 
	 * @return
	 */
	public synchronized Collection<Map.Entry<K,V>> getAll() {
		return new ArrayList<Map.Entry<K,V>>(map.entrySet());
	}
}

