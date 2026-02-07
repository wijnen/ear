/*
Start of a filesViewer class, which should be used in a future trackViewer class, that will allow advanced file-system manipulation of the tracks. Currently not included, as I assume that ear will be run from a local PC anyway, and if you need to upload new stuff, it's better to do it by moving it to the correct folder in the filesystem. Also, saves on security issues etc on delete
*/


#include <Wt/WFileUpload>
//#include <Wt/WFileDropWidget> //This returns file not found, what's the problem?
#include <Wt/WProgressBar>


WContainerWidget *fileContainer = new WContainerWidget(root());

//TODO: Make multiple upload widgets, if possible using the drag/drop interface or the clicky interface.
//One upload widget should be for media files (as in actual audio or video tracks)
//One should be for a fragments.txt (single file)
//One should be for annotations/
//We should collect all of the filenames of uploaded files into one big JSON opbject, and toss that to the Python code.







class FilesViewer : public Wt::WContainerWidget
{
	public:
		FilesViewer(Wt::WContainerWidget *parent =0, Wt::WString title = "");
		const std::vector<Wt::Http::UploadedFile> uploadedFiles();
		Wt::WText *titleText;
	private:
		Wt::WFileUpload *fu; //If we allow multiple files, we need to do additional magic to not destroy stuff
		const std::vector<Wt::Http::UploadedFile> uploadedFiles;
		const std::vector<Wt::FileResource> localFiles;
/*
Wt::WFileResource *csvFile = new Wt::WFileResource("text/csv", "/opt/files/afile.csv");
csvFile->suggestFileName("data.csv");
Wt::WAnchor *anchor = new Wt::WAnchor(csvFile, "CSV data");
Wt::WFileResource *imageFile = new Wt::WFileResource("image/png", "/opt/files/image.png");
imageFile->suggestFileName("data.png");
Wt::WImage *image = new Wt::WImage(imageFile, "PNG version");
*/
};

const std::vector<Wt::Http::UploadedFile>  OptionalMultiFileUploader::uploadedFiles()
{
	return uploadedFiles;
}
FilesViewer::addLocalFile(WString filename)
{
	localFiles->push_back(Wt::WFileResource(filename);//TODO: Add mimetype
}
FilesViewer::FilesViewer(Wt::WContainerWidget *parent, Wt::WString title) : Wt::WContainerWidget(parent)
{

	Wt::WContainerWidget *titleContainer = new Wt::WContainerWidget(this);
  	titleText = new Wt::WText(title, titleContainer);
	Wt::WContainerWidget *uploadersContainer = new Wt::WContainerWidget(this);
	Wt::WContainerWidget *listContainer = new Wt::WContainerWidget(this);
	Wt::WContainerWidget *container = new Wt::WContainerWidget(uploadersContainer);

	Wt::WFileUpload *fu = new Wt::WFileUpload(container);
	fu->setFileTextSize(50); // Set the width of the widget to 50 characters
	fu->setProgressBar(new Wt::WProgressBar());
	fu->setMargin(10, Wt::Right);
	fu->setMultiple(true); //Optional TODO, make this accesible outside the method

	// Upload automatically when the user entered a file.
	fu->changed().connect(std::bind([=] () {
	    fu->upload();
	    uploadButton->disable();
	    out->setText("File upload is changed.");
	}));

	// React to a succesfull upload.
	fu->uploaded().connect(std::bind([=] () {
	    out->setText("File upload is finished.");
	    for(auto:file:fu->uploadedFiles()) //This part should be copied to the drop zone, once it works
	    {
		if (std::find(uploadedFiles.begin(), uploadedFiles.end(),file)!=uploadedFiles.end()) //If file not in filesF
		{
			this->uploadedFiles.push_back(file);
			file->stealSpoolFile();
		}
            }
	    listContainer->clear()
	    for(auto file:this->files)
	    {
		listContainer->addWidget(Wt::WText(file->clientFileName()));
	    }
	}));

	// React to a file upload problem.
	fu->fileTooLarge().connect(std::bind([=] () {
	    out->setText("File is too large.");
	}));

/*
	Wt::WFileDropWidget *dropWidget = new Wt::WFileDropWidget(uploadersContainer);

	dropWidget->drop().connect(std::bind([=] (const std::vector<Wt::WFileDropWidget::File*>& files) {
	  const int maxFiles = 5;
	  unsigned prevNbFiles = dropWidget->uploads().size() - files.size();
	  for (unsigned i=0; i < files.size(); i++) {
	    if (prevNbFiles + i >= maxFiles) {
	      dropWidget->cancelUpload(files[i]);
	      continue;
	    }

	    Wt::WContainerWidget *block = new Wt::WContainerWidget(dropWidget);
	    block->setToolTip(files[i]->clientFileName());
	    block->addStyleClass("upload-block spinner");
	  }

	  if (dropWidget->uploads().size() >= maxFiles)
	    dropWidget->setAcceptDrops(false);
	}, std::placeholders::_1));

	dropWidget->uploaded().connect(std::bind([=] (Wt::WFileDropWidget::File* file) {
	  std::vector<Wt::WFileDropWidget::File*> uploads = dropWidget->uploads();
	  std::size_t idx = 0;
	  for (; idx != uploads.size(); ++idx)
	    if (uploads[idx] == file)
	      break;
	  dropWidget->widget(idx)->removeStyleClass("spinner");
	  dropWidget->widget(idx)->addStyleClass("ready");
	}, std::placeholders::_1));

	dropWidget->tooLarge().connect(std::bind([=] (Wt::WFileDropWidget::File *file) {
	  std::vector<Wt::WFileDropWidget::File*> uploads = dropWidget->uploads();
	  std::size_t idx = 0;
	  for (; idx != uploads.size(); ++idx)
	    if (uploads[idx] == file)
	      break;
	  dropWidget->widget(idx)->removeStyleClass("spinner");
	  dropWidget->widget(idx)->addStyleClass("failed");
	}, std::placeholders::_1));

	dropWidget->uploadFailed().connect(std::bind([=] (Wt::WFileDropWidget::File *file) {
	  std::vector<Wt::WFileDropWidget::File*> uploads = dropWidget->uploads();
	  std::size_t idx = 0;
	  for (; idx != uploads.size(); ++idx)
	    if (uploads[idx] == file)
	      break;
	  dropWidget->widget(idx)->removeStyleClass("spinner");
	  dropWidget->widget(idx)->addStyleClass("failed");
	}, std::placeholders::_1));*/ //This header file is missing, possibly available in new version of |Wt?

}


