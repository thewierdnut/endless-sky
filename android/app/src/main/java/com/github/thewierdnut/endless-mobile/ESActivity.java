package com.github.thewierdnut.endless_mobile;


import org.libsdl.app.SDLActivity;
import android.content.Intent;
import java.io.OutputStream;
import java.io.InputStream;
import java.io.BufferedInputStream;
import android.net.Uri;
import java.io.IOException;
import android.app.Activity;
import android.widget.Toast;
import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.File;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipEntry;

/**
    SDL Activity.
*/
public class ESActivity extends SDLActivity
{
    static int SAVE_FILE = 1;
    static int GET_FILE = 2;
    static int UNZIP_FILE = 3;

    protected String[] getLibraries()
    {
        return new String[] {
            // Everything is statically linked
            // "SDL2",
            // "SDL2_image",
            // "SDL2_mixer",
            // "SDL2_net",
            // "SDL2_ttf",
            "main"
        };
    }

    // Call to save a file using an intent
    protected void saveFile(String filename, byte[] content)
    {
        save_file_content = content;
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("text/plain");
        intent.putExtra(Intent.EXTRA_TITLE, filename);
        startActivityForResult(intent, SAVE_FILE);
    }

    // Call to request a file using an intent
    protected byte[] getFile(String prompt, String mime_type)
    {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType(mime_type);
        loaded_file_content = null;
        synchronized(load_file_lock)
        {
            load_file_lock[0] = 0;
            startActivityForResult(Intent.createChooser(intent, prompt), GET_FILE);
            //startActivityForResult(intent, GET_FILE);
            // wait until intent finishes
            try
            {
                while (0 == load_file_lock[0])
                {
                    load_file_lock.wait();
                }
            }
            catch(InterruptedException e)
            {
                return null;
            }
        }
        byte[] ret = loaded_file_content;
        loaded_file_content = null;
        return ret;
    }

    // Call to unzip a large file
    protected boolean promptUserAndUnzipFile(String prompt, String path)
    {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("application/zip");
        synchronized(zip_file_lock)
        {
            unzip_path = path;
            zip_file_lock[0] = 0;
            startActivityForResult(Intent.createChooser(intent, prompt), UNZIP_FILE);
            //startActivityForResult(intent, UNZIP_FILE);
            // wait until intent finishes
            try
            {
                while (0 == zip_file_lock[0])
                {
                    zip_file_lock.wait();
                }
            }
            catch(InterruptedException e)
            {
                return false;
            }
        }
        return zip_file_lock[0] == 1;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        if(requestCode == SAVE_FILE && resultCode == Activity.RESULT_OK)
        {
            Uri uri = data.getData();
            try
            {
                OutputStream output = getContext().getContentResolver().openOutputStream(uri);

                output.write(save_file_content);
                output.flush();
                output.close();
                save_file_content = null;
            }
            catch(IOException e)
            {
                Toast.makeText(this, "Endless-mobile unable to save file to " + uri.getEncodedPath(), Toast.LENGTH_SHORT).show();
            }
        }
        else if(requestCode == GET_FILE)
        {
            if (resultCode == Activity.RESULT_OK)
            {
                Uri uri = data.getData();
                try
                {
                    InputStream input = getContext().getContentResolver().openInputStream(uri);
                    ByteArrayOutputStream bs = new ByteArrayOutputStream();
                    byte[] buffer = new byte[4096];
                    int n = 0;
                    while ((n = input.read(buffer)) != -1)
                    {
                        bs.write(buffer, 0, n);
                    }
                    loaded_file_content = bs.toByteArray();
                    input.close();
                }
                catch(IOException e)
                {
                    loaded_file_content = null;
                    Toast.makeText(this, "Endless-mobile unable to open " + uri.getEncodedPath(), Toast.LENGTH_SHORT).show();
                }
            }
            // unblock the getFile function
            synchronized(load_file_lock)
            {
                load_file_lock[0] = 1;
                load_file_lock.notify();
            }
        }
        else if(requestCode == UNZIP_FILE)
        {
            int status = 0;
            if (resultCode == Activity.RESULT_OK)
            {
                Uri uri = data.getData();
                try
                {
                    InputStream is = getContext().getContentResolver().openInputStream(uri);
                    ZipInputStream zip = new ZipInputStream(new BufferedInputStream(is));
                    ZipEntry e;
                    while ((e = zip.getNextEntry()) != null)
                    {
                        String path = unzip_path + e.getName();
                        if (e.isDirectory())
                        {
                            File f = new File(path);
                            f.mkdirs();
                        }
                        else
                        {
                            FileOutputStream os = new FileOutputStream(path);
                            byte[] buffer = new byte[8192];
                            int len;
                            while ((len = zip.read(buffer)) != -1)
                            {
                                os.write(buffer, 0, len);
                            }
                            os.close();
                        }
                        zip.closeEntry();
                    }

                    zip.close();
                    status = 1; // success
                }
                catch(IOException e)
                {
                    status = 2; // failure
                    Toast.makeText(this, "Endless-mobile unable to open " + uri.getEncodedPath(), Toast.LENGTH_SHORT).show();
                }
            }
            else
            {
                status = 2; // failure
            }
            // unblock the getFile function
            synchronized(zip_file_lock)
            {
                zip_file_lock[0] = status;
                zip_file_lock.notify();
            }
        }
    }

    byte[] save_file_content;
    byte[] loaded_file_content;
    int[] load_file_lock = new int[1];
    int[] zip_file_lock = new int[1]; // 0 pending, 1 success, 2 failure
    String unzip_path;
}

