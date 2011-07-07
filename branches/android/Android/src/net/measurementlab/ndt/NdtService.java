package net.measurementlab.ndt;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class NdtService extends Service {
	public static final int PREPARING = 0;
	public static final int UPLOADING = 1;
	public static final int DOWNLOADING = 2;
	public static final int COMPLETE = 3;
	
	public static final String INTENT_UPDATE_STATUS = "net.measurementlab.ndt.UpdateStatus";
	public static final String EXTRA_STATE = "status";
	
	private Intent intent;
	
	private String networkType;
	
	private boolean testRunning;

	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}

	@Override
	public void onCreate() {
		super.onCreate();
		Log.i("ndt", "Service created.");
		intent = new Intent(INTENT_UPDATE_STATUS);
	}

	@Override
	public void onStart(Intent intent, int startId) {
		Log.i("ndt", "Starting NDT service.");
		super.onStart(intent, startId);
		if (testRunning) {
			return;
		}
		if (null != intent) {
			networkType = intent.getStringExtra("networkType");
		}
		try {
			testRunning = true;
			new Thread(new NdtTests(
					Constants.SERVER_HOST[Constants.DEFAULT_SERVER],
					new CaptiveUiServices(), networkType)).start();
		} catch (Throwable tr) {
			Log.e("ndt", "Problem running tests.", tr);
		}
	}
	
	@Override
	public void onDestroy() {
		super.onDestroy();
		Log.i("ndt", "Finishing NDT service.");
	}

	private class CaptiveUiServices implements UiServices {

		@Override
		public void appendString(String str, int viewId) {
			Log.d("ndt", String.format("Appended: (%1$d) %2$s.", viewId, str
					.trim()));

			if (str.contains("client-to-server") && 0 == viewId) {
				Log.i("ndt", "Starting upload test.");
				intent.putExtra("status", UPLOADING);
				sendBroadcast(intent);
				Log.i("ndt", "Broadcast status change.");
			}

			if (str.contains("server-to-client") && 0 == viewId) {
				Log.i("ndt", "Starting download test.");
				intent.putExtra("status", DOWNLOADING);
				sendBroadcast(intent);
				Log.i("ndt", "Broadcast status change.");
			}
		}

		@Override
		public void incrementProgress() {
			Log.d("ndt", "Incremented progress.");
		}

		@Override
		public void logError(String str) {
			Log.e("ndt", String.format("Error: %1$s.", str.trim()));
		}

		@Override
		public void onBeginTest() {
			Log.d("ndt", "Test begun.");
		}

		@Override
		public void onEndTest() {
			Log.d("ndt", "Test ended.");
			intent.putExtra("status", COMPLETE);
			sendBroadcast(intent);
			Log.i("ndt", "Broadcast status change.");
		}

		@Override
		public void onFailure(String errorMessage) {
			Log.d("ndt", String.format("Failed: %1$s.", errorMessage));
		}

		@Override
		public void onLoginSent() {
			Log.d("ndt", "Login sent.");
		}

		@Override
		public void onPacketQueuingDetected() {
			Log.d("ndt", "Packet queuing detected.");
		}

		@Override
		public void setVariable(String name, int value) {
			Log.d("ndt", String.format(
					"Setting variable, %1$s, to value, %2$d.", name, value));
		}

		@Override
		public void setVariable(String name, double value) {
			Log.d("ndt", String.format(
					"Setting variable, %1$s, to value, %2$f.", name, value));
		}

		@Override
		public void setVariable(String name, Object value) {
			Log.d("ndt", String.format(
					"Setting variable, %1$s, to value, %2$s.", name,
					(null == value) ? "null" : value.toString()));
		}

		@Override
		public void updateStatus(String status) {
			Log.d("ndt", String.format("Updating status: %1$s.", status));
		}

		@Override
		public void updateStatusPanel(String status) {
			Log.d("ndt", String.format("Updating status panel: %1$s.", status));
		}

		@Override
		public boolean wantToStop() {
			return false;
		}

	}
}
