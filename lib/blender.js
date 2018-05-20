var spawn = require('child_process').spawn;

var temp = require('temp');
var BLENDER_SCRIPT_PATH = './blendscripts/';

// FIXME a lot of this is rancid copypasta with differing script names

function run (python_script, headless, project_file, env, done) {

  if (headless === undefined) {
    headless = true;
  }

  var args = [];
  var stdout = "";
  var stderr = "";

  if (project_file) {
    args.push(project_file);
  }

  if (python_script) {
    args.push("-P");
    args.push(python_script);
  }

  if (headless) {
    args.push("-b");
  }

  if (!env) {
    env = {};
  }

  var blender = spawn("blender", args, {env: env});

  blender.stdout.setEncoding('utf8');
  blender.stdout.on('data', function(data) {
    stdout += data;
  });
  blender.stderr.setEncoding('utf8');
  blender.stderr.on('data', function(data) {
    stderr += data;
  });
  blender.on('close', function (code) {
    done(null, stdout);
  });
  blender.on('error', function (err) {
    console.error('Failed to start blender.');
    done(err);
  });
}

exports.run = run;

function mesh_thickness (input_file, threshold, done) {
  var output_file = temp.path({prefix: 'thin-face-', suffix: '.stl'});
  run(BLENDER_SCRIPT_PATH + 'render_thin.py', true, null, {
    THICKNESS: threshold,
    INPUT_FILE: input_file,
    OUTPUT_FILE: output_file
  }, function (err) {
    done(err, output_file);
  });
}

exports.mesh_thickness = mesh_thickness;

function registration_target (input_file, rotation, done) {
  var output_file = temp.path({suffix: '.png'});
output_file = "test.png";
  run(BLENDER_SCRIPT_PATH + 'render_reg_target.py', true, null, {
    ROTATION: rotation,
    INPUT_FILE: input_file,
    OUTPUT_FILE: output_file
  }, function (err) {
    done(err, output_file);
  });
}

exports.registration_target = registration_target;

function apply_warp (inputs, blendfile, done) {
  run(BLENDER_SCRIPT_PATH + 'apply_warp.py', true, blendfile, {
    INPUTS: JSON.stringify(inputs),
    OUTPUT: blendfile
  }, function (err) {
    done(err, blendfile);
  });
}

exports.apply_warp = apply_warp;

function scale_foot_length (inputs, blendfile, done) {
  run(BLENDER_SCRIPT_PATH + 'scale_foot_length.py', true, blendfile, {
    INPUTS: JSON.stringify(inputs),
    OUTPUT: blendfile
  }, function (err) {
    done(err, blendfile);
  });
}

exports.scale_foot_length = scale_foot_length;

function read_measurement (inputs, blendfile, done) {
  run(BLENDER_SCRIPT_PATH + 'read_measurement.py', true, blendfile, {
    INPUTS: JSON.stringify(inputs)
  }, function (err, output) {
    done(err, output);
  });
}

exports.read_measurement = read_measurement;

function export_obj (inputs, obj_file, blendfile, done) {
  run(BLENDER_SCRIPT_PATH + 'export_obj.py', true, blendfile, {
    INPUTS: JSON.stringify(inputs),
    OUTPUT: obj_file
  }, function (err, output) {
    done(err, output);
  });
}

exports.export_obj = export_obj;

function export_stl (inputs, stl_file, blendfile, done) {
  run(BLENDER_SCRIPT_PATH + 'export_stl.py', true, blendfile, {
    INPUTS: JSON.stringify(inputs),
    OUTPUT: stl_file
  }, function (err, output) {
    done(err, output);
  });
}

exports.export_stl = export_stl;

function add_path (inputs, blendfile, done) {
  run(BLENDER_SCRIPT_PATH + 'add_path.py', true, blendfile, {
    INPUTS: JSON.stringify(inputs),
    OUTPUT: blendfile
  }, function (err) {
    done(err, blendfile);
  });
}

exports.add_path = add_path;
