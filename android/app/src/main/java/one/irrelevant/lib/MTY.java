package group.matoya.lib;

import androidx.appcompat.app.AppCompatActivity;
import android.view.ViewGroup;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.content.Context;

class MTYAppThread extends Thread {
	native void app_start(String name);

	String name;

	public MTYAppThread(String name) {
		this.name = name;
	}

	public void run() {
		app_start(this.name);
	}
}

class MTYSurface extends SurfaceView implements SurfaceHolder.Callback {
	native void app_dims(int w, int h);
	native void gfx_dims(int w, int h);
	native void gfx_set_surface(Surface surface);
	native void gfx_unset_surface();

	public MTYSurface(Context context) {
		super(context);
        this.getHolder().addCallback(this);
	}

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		app_dims(w, h);
		gfx_dims(w, h);
    }

    public void surfaceCreated(SurfaceHolder holder) {
		gfx_set_surface(holder.getSurface());
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
		gfx_unset_surface();
    }
}

public class MTY {
	native static void gfx_global_init();

	public static void start(String name) {
		System.loadLibrary(name);

		gfx_global_init();

		// Runs the main function on a thread
		MTYAppThread thread = new MTYAppThread(name);
		thread.start();
	}

	public static void setSurface(AppCompatActivity activity, ViewGroup.LayoutParams params) {
		MTYSurface surface = new MTYSurface(activity.getApplicationContext());
		activity.addContentView(surface, params);
	}
}
