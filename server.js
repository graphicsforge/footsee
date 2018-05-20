
var temp = require('temp');
var fs = require('fs-extra');
var async = require('async');
var express = require('express');
var bodyParser = require('body-parser');
var formidable = require('formidable');
var spawn = require('child_process').spawn;
var request = require('request');

var footsee = require("./lib/footsee");
var blender = require('./lib/blender');

var app = express();

// parse request bodies with type application/json
app.use(bodyParser.json({
  limit: '10mb',
  uploadDir: '/tmp'
}));

app.post('/build', function(req, res) {
  var form = new formidable.IncomingForm(),
  files = {},
  fields = {};
  form.on('field', function(field, value) {
    fields[field] = value;
  });
  form.on('file', function(field, file) {
    console.log(file.name);
    files[field] = file;
  });
  form.on('end', function() {
    console.log(JSON.stringify(fields));
    console.log(JSON.stringify(files));
    var convert_plantar = spawn('convert', [files.plantar.path, "-resize", "1280x720!", files.plantar.path]);
    convert_plantar.on('close', function(code, signal) {
      var convert_medial = spawn('convert', [files.medial.path, "-resize", "1280x720!", files.medial.path]);
      convert_medial.on('close', function(code, signal) {
        footsee.build({
          "plantar": files.plantar.path,
          "medial": files.medial.path,
          "rulerless": files.rulerless.path,
          "foot_length": parseFloat(fields.foot_length),
          "side": fields.side
        }, function(err, data) {
          // clean up upload paths
          fs.unlink(files.plantar.path);
          fs.unlink(files.medial.path);
          fs.unlink(files.rulerless.path);
          if (err) return res.status(500).end(err);

          console.log(data);

          if (fields.debug=="1") {
            req.registration = data;
            return debug(req, res);
          }

          var output_base_filename = "model";

          var blend_path = temp.path({suffix: '.blend'});
          var transform_mesh = [
            // make a copy of the things model
            fs.copy.bind(fs, "./models/things.blend", blend_path),
            blender.apply_warp.bind(blender, data, blend_path)
          ];

          if (fields.side=="left") {
            transform_mesh.push(blender.apply_warp.bind(blender, {warp: [[-1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]}, blend_path));
          }

          // optionally engrave label
          if (fields.label) {
            output_base_filename = fields.label;
            transform_mesh.push(function(callback) {
              strToImg(fields.label, function(err, img_path) {
                console.log("handwriting.io "+img_path);
                imgToSvg(img_path, function(err, svg_path) {
                  console.log("svg "+svg_path);
                  blender.add_path({model_name: fields.model_name, svg_path: svg_path}, blend_path, function(err, output) {
                    callback(null, output);
                  });
                }); // imgToSvg
              }); // strToImg
            });
          }

          // optionally export specific component
          if (fields.model_name) {
            var stl_path = temp.path({suffix: '.stl'});
            transform_mesh.push(blender.export_stl.bind(blender, {"model_name":fields.model_name}, stl_path, blend_path));
          }

          async.series(transform_mesh, function(err, filenames) {
            if (fields.model_name) {
              res.setHeader('Content-disposition', `attachment; filename=${output_base_filename}.stl`);
              fs.readFile(stl_path, function(err, file) {
                res.end(file);
                fs.unlink(stl_path);
              });
            } else {
              res.setHeader('Content-disposition', `attachment; filename=${output_base_filename}.blend`);
              fs.readFile(blend_path, function(err, file) {
                res.end(file);
                fs.unlink(blend_path);
              });
            }
            fs.unlink(data.output);
          });

        }); // footsee.build
      });
    });
  }); // form end
  form.parse(req);
});

app.get('/:any.jpg', function(req, res) {
  fs.readFile(req.params.any+".jpg", function(err, data) {
    if (err) return res.status(404);
    res.end(data);
  });
});

app.get('/', function(req, res) {
  res.end(`
<style>
  label {
  }
</style>
<form action="build" method="POST" enctype="multipart/form-data" >
  <label>label</label>
  <input type="text" name="label" /></br>
  <label>medial</label>
  <input type="file" name="medial" /></br>
  <label>plantar</label>
  <input type="file" name="plantar" /></br>
  <label>side</label>
  <select name="side">
    <option value="left">left</option>
    <option value="right">right</option>
  </select>
  <hr>
  <label>ruler(less) measurement</label>
  <input type="file" name="rulerless" /></br>
  <label>or enter a foot length</label>
  <input type="text" name="foot_length" />
  <hr>
  <select name="debug">
    <option value="1">debug mode</option>
    <option value="0">download model</option>
  </select>
  <label>download_options</label>
  <select name="model_name">
    <option value="">download everything</option>
    <option value="footbed">download insole</option>
    <option value="john_arch_footbed">download john arch</option>
  </select>
  <hr>
  <input type="submit" value="upload" />
</form>
`);
});

var server = app.listen(80, function () {
});


var debug = function(req, res) {

  blender.read_measurement({measurement: "foot_length"}, req.registration.output, function(err, measurement) {
    res.setHeader('Content-disposition', 'text/HTML');
    res.end(`
      <html>
        ${measurement}
        <img src="rulerless_seg.jpg" />
        <img src="medial_sagittal_warped.jpg" />
        <img src="plantar_transverse_warped.jpg" />
      </html>
    `);
  });

}

// create a svg from an image
var imgToSvg = function(img_path, callback) {
  var pnm_path = temp.path({suffix: '.pnm'});
  var svg_path = temp.path({suffix: '.svg'});
  var convert = spawn('convert', [img_path, "-alpha", "remove", "-resize", "256x256", pnm_path]);
  convert.on('close', function(code, signal) {
    var potrace = spawn('potrace', ["--svg", "-k", "0.5", pnm_path, "-o", svg_path]);
    potrace.on('close', function(code, signal) {
      callback(null, svg_path);
    });
  });
};

// create an image from a string
var strToImg = function(string, callback) {
  var image_path = temp.path({suffix: '.png'});
  var writeStream = fs.createWriteStream(image_path);
  request(`https://9YVSD2H8A9DCK8A8:DNZCYBZSCP83N31F@api.handwriting.io/render/png/?handwriting_id=31SB2CWG00DZ&handwriting_size=70px&line_spacing=1.5&width=720px&height=470px&text=${string}`).pipe(writeStream);
  writeStream.on('finish', function() {
    callback(null, image_path);
  });
};
