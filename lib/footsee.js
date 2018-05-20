
var temp = require('temp');
var fs = require('fs-extra');
var async = require('async');
var spawn = require('child_process').spawn;
var math = require('mathjs');

var blender = require('./blender');

function runRegistration(image_path, side, reg_target_path, type, callback) {
  var time_start = Date.now();
  var mask_path = "mask_plantar.png";
  if (type == "medial_sagittal") {
    mask_path = "mask_medial.png";
  }
  var register = spawn("./build/register", [
    "-i", image_path, "-r", reg_target_path, "-m", mask_path, "-t", type, "-s", side, "-d", type
  ]);
  var register_out = "";
  var register_error = "";
  register.stdout.setEncoding('utf8');
  register.stdout.on('data', function(data) {
    register_out += data;
  });
  register.stderr.setEncoding('utf8');
  register.stderr.on('data', function(data) {
    register_error += data;
  });
  register.on('close', function (ret_code) {
    if (ret_code) {
      return callback("register returned "+ret_code+" "+register_error);
    }
    try {
      register_out = JSON.parse(register_out);
    } catch(err) {
      console.log("error with registration");
      console.log(register_out);
      console.log(register_error);
      return callback("malformed response "+err);
    }
    register_out.profile_registration = (Date.now()-time_start)/1000;
    callback(null, register_out);
  });
  register.on('error', function (err) {
    callback(err);
  });
}

function runRulerless(image_path, callback) {
  var time_start = Date.now();
  var rulerless = spawn("./build/rulerless", [
    "-i", image_path, "-d", "rulerless"
  ]);
  var rulerless_out = "";
  var rulerless_error = "";
  rulerless.stdout.setEncoding('utf8');
  rulerless.stdout.on('data', function(data) {
    rulerless_out += data;
  });
  rulerless.stderr.setEncoding('utf8');
  rulerless.stderr.on('data', function(data) {
    rulerless_error += data;
  });
  rulerless.on('close', function (ret_code) {
    if (ret_code) {
      return callback("rulerless returned "+ret_code+" "+rulerless_error);
    }
    try {
      rulerless_out = JSON.parse(rulerless_out);
    } catch(err) {
      console.log("error with rulerless");
      console.log(rulerless_out);
      console.log(rulerless_error);
      return callback("malformed response "+err);
    }
    rulerless_out.profile_rulerless = (Date.now()-time_start)/1000;
    callback(null, rulerless_out);
  });
  rulerless.on('error', function (err) {
    callback(err);
  });

}

// apply a warp matrix and render a new registration target
function applyPlantarRegistrationWarp(data, callback) {
  var blend_path = temp.path({suffix: '.blend'});
  var transform_mesh = [
    // make a copy of the base model
    fs.copy.bind(fs, "./models/foot.blend", blend_path),
    blender.apply_warp.bind(blender, data, blend_path),
    // render the side-view
    blender.registration_target.bind(blender, blend_path, Math.PI/2.0)
  ];

  async.series(transform_mesh, function(err, filenames) {
    callback(err, filenames);
  });
}

function measureFoot(plantar_image, medial_image, side, target_metric, target_measurement, callback) {
  var ret = {"side": side, "target":{"metric":target_metric, "measurement":target_measurement}};
  runRegistration(plantar_image, side, "regtarget_plantar.png", "plantar_transverse", function(err, data) {
    if (err) return callback(err);

    for (var key in data) {
      if (data.hasOwnProperty(key)) {
        ret["plantar_"+key] = data[key];
      }
    }
    applyPlantarRegistrationWarp(data, function(err, filenames) {
      if (err) return callback(err);
      var blend_path = filenames[1];
      var medial_reg_target = filenames[2];
      runRegistration(medial_image, side, medial_reg_target, "medial_sagittal", function(err, data) {
        if (err) return callback(err);

        for (var key in data) {
          if (data.hasOwnProperty(key)) {
            ret["medial_"+key] = data[key];
          }
        }
        ret.warp = math.multiply(ret.medial_warp, ret.plantar_warp);

        blender.apply_warp(data, blend_path, function(err, filename) {
          var finalize_tasks = [
            blender.scale_foot_length.bind(blender, {
              "measurement":target_metric,
              "target_distance":target_measurement
            }, blend_path)
          ];
          if (side=="left") {
            finalize_tasks.push(blender.apply_warp.bind(blender, {warp: [[-1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]}, blend_path));
          }
          async.series(finalize_tasks, function(err) {
            ret.output = blend_path;  
            callback(err, ret);
          });
        });
      });
    });
  });
}

exports.build = function(inputs, callback) {
  var plantar_image = inputs.plantar;
  var medial_image = inputs.medial;
  var rulerless_image = inputs.rulerless;
  var foot_length = inputs.foot_length;
  var side = inputs.side;
  if (typeof(plantar_image)=="undefined" || typeof(medial_image)=="undefined" || typeof(side)=="undefined" || (typeof(rulerless_image)=="undefined" && typeof(foot_length)=="undefined")) {
    return callback("Missing required input");
  }

  // if we have a foot length, just use it
  if (inputs.foot_length) {
    return measureFoot(plantar_image, medial_image, side, "foot_length", foot_length, function(err, data) {
      callback(err, data);
    });
  }

  // otherwise do rulerless measurement
  runRulerless(rulerless_image, function(err, rulerless) {
    measureFoot(plantar_image, medial_image, side, "ball_width", rulerless.width, function(err, data) {
      data.rulerless_width = rulerless.width;
      callback(err, data);
    });
  });
}

// for debuggin/testing, only run when called explicitly from the command line
if (require.main === module) {
  var argv = require("minimist")(process.argv.slice(2));
  if (!argv.plantar_image) {
    console.log("no --plantar_image specified");
  }
}
