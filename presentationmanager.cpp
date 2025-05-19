#include "presentationmanager.h"
#include <QDebug>
#include <QFileInfo>
#include <QPdfPageNavigator>
#include <QElapsedTimer>


PresentationManager::PresentationManager(QObject *parent)
    : QObject(parent)
    , m_document(new QPdfDocument(this))
    , m_pdfView(nullptr)
    , m_currentPage(0)
    , m_totalPages(0)
    , m_zoomLevel(1.0)
{
}

PresentationManager::~PresentationManager()
{
    // Document is deleted by Qt's parent-child mechanism
}

bool PresentationManager::loadPresentation(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        emit error(QString("Cannot read file: %1").arg(filePath));
        return false;
    }

    // Close any previously open document
    m_document->close();

    // Load the new document
    auto result = m_document->load(filePath);
    if (result != QPdfDocument::Error::None) {
        QString errorMessage;
        switch (result) {
        case QPdfDocument::Error::InvalidFileFormat:
            errorMessage = "Invalid PDF format";
            break;
        case QPdfDocument::Error::IncorrectPassword:
            errorMessage = "PDF is password protected";
            break;
        case QPdfDocument::Error::UnsupportedSecurityScheme:
            errorMessage = "Unsupported security scheme";
            break;
        default:
            errorMessage = "Unknown error loading PDF";
        }

        emit error(errorMessage);
        return false;
    }

    // Store file info
    m_filePath = filePath;
    m_presentationTitle = fileInfo.fileName();
    m_totalPages = m_document->pageCount();
    m_currentPage = 0;

    // Reset zoom level
    m_zoomLevel = 1.0;

    // Clear highlights
    m_highlights.clear();

    // Configure PDF view
    if (m_pdfView) {
        m_pdfView->setDocument(m_document);
        m_pdfView->setZoomFactor(m_zoomLevel);
        m_pdfView->setPageMode(QPdfView::PageMode::SinglePage);
        //m_pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
        goToSlide(0); // Go to first page
    }

    emit presentationLoaded(m_presentationTitle, m_totalPages);
    updatePageIndicators();

    qDebug() << "Loaded presentation:" << m_presentationTitle << "with" << m_totalPages << "pages at path:" << m_filePath;
    return true;
}

void PresentationManager::setPdfView(QPdfView *view)
{
    m_pdfView = view;

    // If a document is already loaded, set it on the view
    if (m_pdfView && m_document && !m_filePath.isEmpty()) {
        m_pdfView->setDocument(m_document);
        m_pdfView->setZoomFactor(m_zoomLevel);
        m_pdfView->setPageMode(QPdfView::PageMode::SinglePage);
        //m_pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
        goToSlide(m_currentPage);
    }
}

void PresentationManager::nextSlide()
{
    static QElapsedTimer lastCallTime;
    if (lastCallTime.isValid() && lastCallTime.elapsed() < 200) {  // 200ms içinde ikinci çağrıyı yok say

        return;
    }

    lastCallTime.restart();
    qDebug() << "NextSlide accepted";

    if (isValidPage(m_currentPage + 1)) {
        goToSlide(m_currentPage + 1);
    }
}

void PresentationManager::previousSlide()
{
    static QElapsedTimer lastCallTime;
    if (lastCallTime.isValid() && lastCallTime.elapsed() < 200) {  // 200ms içinde ikinci çağrıyı yok say

        return;
    }

    lastCallTime.restart();

    if (isValidPage(m_currentPage - 1)) {
        goToSlide(m_currentPage - 1);
    }
}

void PresentationManager::goToSlide(int pageNumber)
{
    if (!m_document || !m_pdfView) {
        return;
    }

    // Check if the page number is valid
    if (pageNumber >= 0 && pageNumber < m_document->pageCount()) {
        m_currentPage = pageNumber;

        // Sayfa değişimi pageNavigator üzerinden yapılır
        if (m_pdfView->pageNavigator()) {
            m_pdfView->pageNavigator()->jump(m_currentPage, QPoint());
        }

        emit pageChanged(m_currentPage, m_document->pageCount());
    }
}




void PresentationManager::zoomIn()
{
    static QElapsedTimer lastCallTime;
    if (lastCallTime.isValid() && lastCallTime.elapsed() < 200) {  // 200ms içinde ikinci çağrıyı yok say

        return;
    }

    lastCallTime.restart();

    qDebug() << "zoomin";
    setZoomLevel(m_zoomLevel * 1.25);
}

void PresentationManager::zoomOut()
{
    static QElapsedTimer lastCallTime;
    if (lastCallTime.isValid() && lastCallTime.elapsed() < 200) {  // 200ms içinde ikinci çağrıyı yok say

        return;
    }

    lastCallTime.restart();

    setZoomLevel(m_zoomLevel * 0.8);
}

void PresentationManager::setZoomLevel(qreal zoomFactor)
{
    // Limit zoom range
    if (zoomFactor < 0.25) zoomFactor = 0.25;
    if (zoomFactor > 5.0) zoomFactor = 5.0;

    m_zoomLevel = zoomFactor;

    if (m_pdfView) {
        m_pdfView->setZoomFactor(m_zoomLevel);
    }

    emit zoomChanged(m_zoomLevel);
}

void PresentationManager::highlight(const QRect &area)
{
    m_highlights.append(area);

    // Here you would implement the actual highlight rendering
    // This depends on your UI implementation, but could involve
    // adding a transparent colored rectangle over the PDF view

    emit highlightApplied(area);
}

void PresentationManager::clearHighlights()
{
    m_highlights.clear();
    emit highlightsCleared();
}

int PresentationManager::getCurrentPage() const
{
    return m_currentPage;
}

int PresentationManager::getTotalPages() const
{
    return m_totalPages;
}

qreal PresentationManager::getZoomLevel() const
{
    return m_zoomLevel;
}

QString PresentationManager::getPresentationTitle() const
{
    return m_presentationTitle;
}

void PresentationManager::updatePageIndicators()
{
    emit pageChanged(m_currentPage, m_totalPages);
}

bool PresentationManager::isValidPage(int page) const
{
    return (page >= 0 && page < m_totalPages);
}
