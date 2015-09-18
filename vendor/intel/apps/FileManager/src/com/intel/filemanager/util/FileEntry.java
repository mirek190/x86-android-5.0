package com.intel.filemanager.util;

import java.io.File;
import java.util.HashMap;



    public class FileEntry extends HashMap<String, String> implements Comparable<FileEntry>
    {
    	private static final long serialVersionUID = 7526472295622776147L;
    	//TODO add sorted by here.
    	public final static int SORTMOD_NAME=0;
    	public final static int SORTMOD_SIZE=1;
    	public final static int SORTMOD_TYPE=2;
    	public final static int SORTMOD_TIME=3;
    	public final static int SORTMOD_OTHER=10;
    	
    	/**
    	 *these constants is used to index items in map.
    	 */
    	public final static String FILE_TITLE = "title";
    	public final static String FILE_PATH = "path";
    	public final static String FILE_TYPE = "type";
    	public final static String FILE_TIME = "time";
    	public final static String FILE_SIZE = "size";
    	
    	private static int mSortMode = SORTMOD_OTHER;
    	
    	private boolean isSelected = false;
    	
    	public static void setSortType(int mod)
    	{
    		mSortMode = mod;
    	}
    			
    	public boolean isSelected()
    	{
    		return isSelected;
    	}
    	
    	public void setSelected(boolean selected)
    	{
    		isSelected = selected;
    	}
    	
    	public void setTitle(String title)
    	{
    		put(FILE_TITLE, title);
    	}
    	
    	public String getTitle()
    	{
    		return get(FILE_TITLE);
    	}
    	
    	public void setPath(String path)
    	{
    		put(FILE_PATH, path);
    	}
    	
    	public String getPath()
    	{
    		return get(FILE_PATH);
    	}
    	
    	public void setType(String type)
    	{
    		put(FILE_TYPE, type);
    	}
    	public String getType()
    	{
    		return get(FILE_TYPE);
    	}
    	
    	public void setTime(String time)
    	{
    		put(FILE_TIME, time);
    	}
    	
    	public String getTime()
    	{
    		return get(FILE_TIME);
    	}
    	
    	public void setSize(String size)
    	{
    		put(FILE_SIZE, size);
    	}
    	
    	public String getSize()
    	{
    		return get(FILE_SIZE);
    	}
    	//implement camparable interface.
    	public int compareTo(FileEntry other)
    	{
 //   		Log.d("File Manager", "sort type = " + mSortMode);
    		boolean isSame = true;
    		int rt = 0;
    		String nowpath = this.get(FILE_PATH);
    		String otherpath = other.get(FILE_PATH);
    		File nowfile = new File(nowpath);
    		File otherfile=new File(otherpath);
    		
    		if(nowfile.isDirectory()^otherfile.isDirectory() == true)
    			isSame = false;
    		
    		switch(mSortMode)
    		{
    		case SORTMOD_TIME:
   				if(isSame)
   				{
   					Long nowtime = new Long(this.get(FILE_TIME));
   					Long othertime= new Long(other.get(FILE_TIME));
   					rt = -nowtime.compareTo(othertime);//the last modified file  comes firstly.
   				}
   				else
   					rt = nowfile.isDirectory()?-1:1;
    			break;
    		case SORTMOD_TYPE:
    				if(!isSame)
    					rt = nowfile.isDirectory()?-1:1;
    				else if(nowfile.isDirectory())
    					rt = nowpath.compareTo(otherpath);
    				else
    				{
    					String nowtype = new String(this.get(FILE_TYPE));
    					String othertype = new String(other.get(FILE_TYPE));
    					rt = nowtype.compareTo(othertype);
    				}
    			break;
    		case SORTMOD_SIZE:
    				if(!isSame)
    					rt = nowfile.isDirectory()?-1:1;
    				else if(nowfile.isDirectory())
    					rt = nowpath.compareTo(otherpath);
    				else
    				{
    					Long thissize = new Long(this.get(FILE_SIZE));
    					Long othersize = new Long(other.get(FILE_SIZE));
    					rt = -thissize.compareTo(othersize);
    				}
    			break;
    		//default sort type is NAME.
    		case SORTMOD_NAME:
    				if(isSame)
    					rt = nowpath.compareTo(otherpath);
    				else
    					rt = nowfile.isDirectory()?-1:1;
    			break;
    		default:
    			if(this.get(FILE_TITLE) != null)
    			{
    				if(isSame)
    					rt = 0;
    				else
    					rt = nowfile.isDirectory()?-1:1;
    			}
    		}
    		return rt;
    	}
    }
