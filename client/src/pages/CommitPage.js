import React, { useState } from 'react';
import CommitBar from '../components/commits/CommitBar';
import './CommitsPage.css';
import Header from '../components/Header';
import FooterBar from '../components/FooterBar';

const CommitPage = () => {
  const [top, setTop] = useState();
  return (
    <>
      <div className="minheight">
        <Header />
        <div>
          <CommitBar />
        </div>
        <FooterBar />
      </div>
    </>
  );
};

export default CommitPage;
