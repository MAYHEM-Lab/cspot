<?php
ini_set('upload_max_filesize', '20M');
ini_set('post_max_size', '20M');
ini_set('max_input_time', 300);
ini_set('max_execution_time', 300);
ini_set("memory_limit", "20000M");
$target_dir = "uploads/";
$target_file = $target_dir . basename($_FILES["fileToUpload"]["name"]);
$uploadOk = 1;
$imageFileType = strtolower(pathinfo($target_file,PATHINFO_EXTENSION));
//echo "file: $target_file type: $imageFileType\n";
// Check if image file is a actual image or fake image
if(isset($_POST["submit"])) {
    $check = getimagesize($_FILES["fileToUpload"]["tmp_name"]);
    if($check !== false) {
        echo "File is an image - " . $check["mime"] . ".";
        $uploadOk = 1;
    } else {
        echo "File is not an image.";
        $uploadOk = 0;
    }
}
// Check if file already exists
//if (file_exists($target_file)) {
//   echo "Sorry, $target_file already exists.";
//  $uploadOk = 0;
//}
// Check file size
if ($_FILES["fileToUpload"]["size"] > 50000000) {
    echo "Sorry, your file is too large.";
    $uploadOk = 0;
}
// Allow certain file formats
if($imageFileType != "jpg" && $imageFileType != "png" && $imageFileType != "jpeg"
&& $imageFileType != "gif" ) {
//    echo "filetype: $imageFileType size: $check";
    echo "Sorry, only JPG, JPEG, PNG & GIF files are allowed.";
    $uploadOk = 0;
}
// Check if $uploadOk is set to 0 by an error
if ($uploadOk == 0) {
//    $start_time = $_SERVER["REQUEST_TIME_FLOAT"];
//    $end_time = microtime(true);
//    $fsize = $_FILES["fileToUpload"]["size"];
//    $bandwidth=0
    echo "Sorry, your file was not uploaded.";
//    echo "size: $fsize elapsed time:  $elapsed_time, bw: $bandwidth";
// if everything is ok, try to upload file
} else {
    if (move_uploaded_file($_FILES["fileToUpload"]["tmp_name"], $target_file)) {
        echo "The file ". basename( $_FILES["fileToUpload"]["name"]). " has been uploaded.\n";
    } else {
        echo "Sorry, there was an error uploading your file.";
    }
    $start_time = $_SERVER["REQUEST_TIME_FLOAT"];
    $end_time = microtime(true);
    $elapsed_time = $end_time - $start_time;
    $bandwidth = 8*($_FILES["fileToUpload"]["size"] / $elapsed_time)/1000000.0;
    $fsize = $_FILES["fileToUpload"]["size"];
    echo "size: $fsize elapsed time:  $elapsed_time, bw: $bandwidth";
}
?>

