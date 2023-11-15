using iText.Kernel.Colors;
using iText.Kernel.Pdf;
using iText.Kernel.Pdf.Canvas;
using iText.Layout;
using iText.Layout.Element;
using iText.Layout.Properties;
using iText.Kernel.Geom;

namespace MP3toWAVConverter
{
    public static class PDFGenerator
    {
        private static readonly Random rand = new();
        private static readonly int marginMM = 20;
        private static int currentPageNum = -1;
        private static PdfCanvas? pdfCanvas = null;
        private static PdfPage? currentPage = null;
        private static readonly Color black = new DeviceCmyk(0.0f, 0.0f, 0.0f, 1.0f);
        private static readonly Color lightGrey = new DeviceCmyk(0.0f, 0.0f, 0.0f, 0.3f);

        private static readonly Color[] borders = { new DeviceCmyk(0.0f, 0.98f, 0.98f, 0.0f),  //red
                                                    new DeviceCmyk(0.0f, 0.47f, 0.98f, 0.01f), //orange
                                                    new DeviceCmyk(0.69f, 0.23f, 0.0f, 0.32f), //blue
                                                    new DeviceCmyk(0.74f, 0.0f, 0.63f, 0.20f), //green
                                                    new DeviceCmyk(0.0f, 1.0f, 0.0f, 0.0f)}; //pink

        private static readonly Color[] fills = { new DeviceCmyk(0.0f, 0.16f, 0.12f, 0.01f),  //pink
                                                  new DeviceCmyk(0.0f, 0.06f, 0.15f, 0.01f),  //yellow
                                                  new DeviceCmyk(0.10f, 0.03f, 0.0f, 0.04f),  //teal
                                                  new DeviceCmyk(0.10f, 0.0f, 0.07f, 0.04f),  //green
                                                  new DeviceCmyk(0.0f, 0.16f, 0.13f, 0.01f)}; //rose

        private static readonly Dictionary<int, int> assignedColors = new();

        private static double MMtoPT_x(double mm, bool applyMargin = true)
        {
            if (applyMargin)
            {
                mm += marginMM;
            }

            double inches = mm / 25.4;
            return inches * 72;
        }

        private static double MMtoPT_y(double mm, bool applyMargin = true)
        {
            if (applyMargin)
            {
                mm += marginMM;
                mm = 279.4 - mm;
            }

            double inches = mm / 25.4;
            return inches * 72;
        }

        private static double MMtoPT_abs(double mm)
        {
            double inches = mm / 25.4;
            return inches * 72;
        }

        private static void DrawFiducial(PdfCanvas pdfCanvas, double xmm, double ymm)
        {
            pdfCanvas.SetStrokeColor(lightGrey)
                .MoveTo(MMtoPT_x(xmm - 3), MMtoPT_y(ymm))
                .LineTo(MMtoPT_x(xmm + 3), MMtoPT_y(ymm))
                .ClosePathStroke();

            pdfCanvas.SetStrokeColor(lightGrey)
                .MoveTo(MMtoPT_x(xmm), MMtoPT_y(ymm - 3))
                .LineTo(MMtoPT_x(xmm), MMtoPT_y(ymm + 3))
                .ClosePathStroke();
        }

        private static void AddPage(PdfDocument pdf)
        {
            assignedColors.Clear();
            currentPage = pdf.AddNewPage();
            pdfCanvas = new(currentPage);
            currentPageNum++;

            if (currentPageNum % 2 == 0)
            {
                DrawFiducial(pdfCanvas, 0, 0);
                DrawFiducial(pdfCanvas, 142.98 + 2.52, 0);
                DrawFiducial(pdfCanvas, 142.98 + 2.52, 194.12);
                DrawFiducial(pdfCanvas, 0, 194.12);

                pdfCanvas.SetStrokeColor(lightGrey)
                      .Circle(MMtoPT_x(6.24), MMtoPT_y(49.06), MMtoPT_abs(3))
                      .ClosePathStroke();
                pdfCanvas.SetStrokeColor(lightGrey)
                      .Circle(MMtoPT_x(6.24), MMtoPT_y(49.06 + 96), MMtoPT_abs(3))
                      .ClosePathStroke();
            }
            else
            {
                DrawFiducial(pdfCanvas, -2.52, 0);
                DrawFiducial(pdfCanvas, 142.98, 0);
                DrawFiducial(pdfCanvas, 142.98, 194.12);
                DrawFiducial(pdfCanvas, -2.52, 194.12);

                pdfCanvas.SetStrokeColor(lightGrey)
                      .Circle(MMtoPT_x(142.98 - 6.24), MMtoPT_y(49.06), MMtoPT_abs(3))
                      .ClosePathStroke();
                pdfCanvas.SetStrokeColor(lightGrey)
                      .Circle(MMtoPT_x(142.98 - 6.24), MMtoPT_y(49.06 + 96), MMtoPT_abs(3))
                      .ClosePathStroke();
            }
        }

        private static void AddSong(int index, SongInfo s)
        {
            if (pdfCanvas == null || currentPage == null)
                return;

            Canvas canvas = new(pdfCanvas, currentPage.GetMediaBox());

            int row = index % 10;
            int col = index / 10;

            double x = currentPageNum % 2 == 0 ? 11.8 : 2.18;
            double y = 2.56;

            int colorIndex = rand.Next(borders.Length);
            int prevXcolor = -1;
            int prevYcolor = -1;

            if (assignedColors.ContainsKey(10 * col + row - 1))
                prevXcolor = assignedColors[10 * col + row - 1];
            if (assignedColors.ContainsKey(10 * (col - 1) + row))
                prevYcolor = assignedColors[10 * (col - 1) + row];

            while (colorIndex == prevXcolor || colorIndex == prevYcolor)
                colorIndex = rand.Next(borders.Length);
            assignedColors.Add(index, colorIndex);

            pdfCanvas.SetStrokeColor(borders[colorIndex])
                  .SetFillColor(borders[colorIndex])
                  .Rectangle(MMtoPT_x(x + col * 65), MMtoPT_y(y + row * 19), MMtoPT_abs(64), -MMtoPT_abs(18))
                  .ClosePathFillStroke();

            pdfCanvas.SetStrokeColor(fills[colorIndex])
                  .SetFillColor(fills[colorIndex])
                  .MoveTo(MMtoPT_x(x + col * 65 + 1.64), MMtoPT_y(y + row * 19 + 1.64))
                  .LineTo(MMtoPT_x(x + col * 65 + 62.36), MMtoPT_y(y + row * 19 + 1.64))
                  .LineTo(MMtoPT_x(x + col * 65 + 62.36), MMtoPT_y(y + row * 19 + 1.64 + 4.61))
                  .LineTo(MMtoPT_x(x + col * 65 + 64 - (9.95 - 2.3)), MMtoPT_y(y + row * 19 + 1.64 + 4.61))
                  .LineTo(MMtoPT_x(x + col * 65 + 64 - 9.95), MMtoPT_y(y + row * 19 + 1.64 + 4.61 + 5.5/2))
                  .LineTo(MMtoPT_x(x + col * 65 + 64 - (9.95 - 2.3)), MMtoPT_y(y + row * 19 + 1.64 + 4.61 + 5.5))
                  .LineTo(MMtoPT_x(x + col * 65 + 62.36), MMtoPT_y(y + row * 19 + 1.64 + 4.61 + 5.5))
                  .LineTo(MMtoPT_x(x + col * 65 + 62.36), MMtoPT_y(y + row * 19 + 16.36))
                  .LineTo(MMtoPT_x(x + col * 65 + 1.64), MMtoPT_y(y + row * 19 + 16.36))
                  .LineTo(MMtoPT_x(x + col * 65 + 1.64), MMtoPT_y(y + row * 19 + 1.64 + 4.61 + 5.5))
                  .LineTo(MMtoPT_x(x + col * 65 + 1.64 + 8.66 - 2.3), MMtoPT_y(y + row * 19 + 1.64 + 4.61 + 5.5))
                  .LineTo(MMtoPT_x(x + col * 65 + 1.64 + 8.66), MMtoPT_y(y + row * 19 + 1.64 + 4.61 + 5.5 / 2))
                  .LineTo(MMtoPT_x(x + col * 65 + 1.64 + 8.66 - 2.3), MMtoPT_y(y + row * 19 + 1.64 + 4.61))
                  .LineTo(MMtoPT_x(x + col * 65 + 1.64), MMtoPT_y(y + row * 19 + 1.64 + 4.61))
                  .LineTo(MMtoPT_x(x + col * 65 + 1.64), MMtoPT_y(y + row * 19 + 1.64))
                  .ClosePathFillStroke();

            Paragraph pID = new(s.BookID);
            pID.SetFontColor(black);
            pID.SetFontSize(11.0f);
            pID.SetBold();
            canvas.ShowTextAligned(pID, (float)MMtoPT_x(x + col * 65 + 4.64), (float)MMtoPT_y(y + row * 19 + 11.5), TextAlignment.CENTER);
            
            Paragraph pYear = new(s.Year);
            pYear.SetFontColor(black);
            pYear.SetFontSize(11.0f);
            canvas.ShowTextAligned(pYear, (float)MMtoPT_x(x + col * 65 + 59.36), (float)MMtoPT_y(y + row * 19 + 11.5), TextAlignment.CENTER);

            Paragraph pName = new(s.Name);
            pName.SetFontColor(black);
            pName.SetFontSize(s.Name.Length > 20 ? 8.0f : 12.0f);
            pName.SetBold();
            canvas.ShowTextAligned(pName, (float)MMtoPT_x(x + col * 65 + 32), (float)MMtoPT_y(y + row * 19 + 8), TextAlignment.CENTER);

            Paragraph pArtist = new(s.Artist);
            pArtist.SetFontColor(black);
            pArtist.SetFontSize(s.Artist.Length > 20 ? 8.0f : 12.0f);
            canvas.ShowTextAligned(pArtist, (float)MMtoPT_x(x + col * 65 + 32), (float)MMtoPT_y(y + row * 19 + 14), TextAlignment.CENTER);

            canvas.Close();
        }

        public static void CreateBooklet(string path, IEnumerable<SongInfo> songList)
        {
            PdfWriter writer = new(path);
            PdfDocument pdf = new(writer);
            Document document = new(pdf, PageSize.LETTER);
            currentPageNum = -1;
            AddPage(pdf);

            for (int i = 0; i < songList.Count(); i++)
            {
                if (i % 20 == 0 && i != 0)
                    AddPage(pdf);
                AddSong(i % 20, songList.ElementAt(i));
            }

            document.Close();
        }
    }
}
