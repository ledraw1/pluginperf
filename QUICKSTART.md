since it is already built then you can run it with the following command:


python3 tools/test_all_plugins.py \
  --vst3-dir /Library/Audio/Plug-Ins/VST3 \
  --output-dir ./plugin_results \
  --buffers 64,256,1024,4096 \
  --iterations 200 \
  --skip-errors

  and the output will be in the plugin_results directory. i left mine in there so you can see. the main thing i was looking for was the
  approx_rt_cpu_pct and dsp_load_pct

  